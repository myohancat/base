/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "CLI.h"

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdarg.h>

#include "NetUtil.h"
#include "Log.h"
#include "vkey.h"

#define IS_QUOTE(ch)   (ch == '"' || ch == '\'')

#define CLI_WELCOME_MSG  "Welcome to the Service ...!"
#define CLI_PROMPT       "DEBUG $ "

#define SEND_TIMEOUT  1000
#define RECV_TIMEOUT  3000

class TelnetSession : public ICliSession
{
public:
    TelnetSession(TcpSession* session) : mSession(session) { };
    ~TelnetSession() { };

    void stop();

    void print(const char* fmt, ...) override;
    void printHistory();

    char* getLine();

    const char* searchHistory(int step);
    void removeHistory();
    void insertHistory(const char* cmd);

    void processVKey(const VKey_t* vkey);

    void doInsertChars(const char* charIn, int charInCnt);
    void doErase(int eraseCnt);
    void doDelete(int deleteCnt);
    void doReturn();
    void doMoveLeft();
    void doMoveRight();
    void doClearLine();

    void reset();

    void setCR(bool value);
    bool isCR();

protected:
    bool mCR = false;
    char mLine[MAX_READ_LINE_LEN];
    int  mCharCnt = 0;
    int  mCaretPos = 0;

    char mCmdLines[MAX_HISTORY_CNT][MAX_READ_LINE_LEN];
    int  mFirstHistory = 0;
    int  mLastHistory  = MAX_HISTORY_CNT - 1;
    int  mCountHistory = 0;
    int  mStepHistory = -1;

    TcpSession* mSession;
};

inline void  TelnetSession::stop() { mSession->stop(); }
inline char* TelnetSession::getLine() { return mLine; }
inline void  TelnetSession::setCR(bool value) { mCR = value; }
inline bool  TelnetSession::isCR() { return mCR; }

static int make_args(const char* command, char*** pargv)
{
    char   buf[MAX_READ_LINE_LEN];
    int    argc = 0;
    char** argv = NULL;
    char*  p = buf, *start = NULL, *end = NULL;

    if (command == NULL)
    {
        *pargv = NULL;
        return 0;
    }

    strncpy(buf, command, MAX_READ_LINE_LEN -1);
    buf[MAX_READ_LINE_LEN -1] = 0;

    while(*p)
    {
        if (!start)
        {
            while(isspace(*p)) p++;
            if (*p == 0) break;
            start = p;
        }
        else
        {
            if (!IS_QUOTE(*start))
            {
                while(*p && !isspace(*p)) p++;
                if (*p == 0) end = p-1;
                else { end = p - 1; *p = 0; }
            }
            else
            {
                while(*p && *start != *p) p++;
                if (*p == 0) end = p-1;
                else { start ++; end = p - 1; *p = 0; }
            }
        }

        p++;

        if (start && (end || *p == 0))
        {
            argv = (char**)realloc(argv, sizeof(char*) * (argc + 1));
            argv[argc++] = strdup(start);
            start = end = NULL;
        }
    }

    if (argv)
    {
        argv = (char**)realloc(argv, sizeof(char*) * (argc + 1));
        argv[argc] = NULL;
    }

    *pargv = argv;

    return argc;
}

static void free_args(char** argv)
{
    int ii;
    if (!argv)
        return;

    for(ii = 0; argv[ii]; ii++)
        free(argv[ii]);

    free(argv);
}

void CLI::cmd_exit(ICliSession* session, int argc, char** argv, void* param)
{
    UNUSED(param);
    UNUSED(argc);
    UNUSED(argv);

    TelnetSession* telnetSession = (TelnetSession*)session;
    telnetSession->stop();
}

CLI& CLI::getInstance()
{
    static CLI _instance;

    return _instance;
}

CLI::CLI()
{
    addCommand("clear",   cmd_clear,   this, "clear screen");
    addCommand("help",    cmd_help,    this, "show help");
    addCommand("history", cmd_history, this, "show history");
    addCommand("exit",    cmd_exit,    this, "exit program");
}

CLI::~CLI()
{
    stop();
}

bool CLI::start()
{
    mServer.setSessionListener(this);
    return mServer.start(CLI_SERVER_PORT);
}

void CLI::stop()
{
    mServer.stop();
    mServer.setSessionListener(NULL);
}

void CLI::onTcpSessionEstablished(TcpSession* session)
{
    static uint8_t charactor_mode[] = {255, 251, 1, 255, 251, 3, 255, 252, 34};

    char line[2048];

    TelnetSession* telnetSession = new TelnetSession(session);
    session->setParam(telnetSession);

    // Mode Setting
    NetUtil::send(session->getFD(), charactor_mode, sizeof(charactor_mode), SEND_TIMEOUT);

    // Flush Response
    NetUtil::recv(session->getFD(), line, 1, RECV_TIMEOUT);
    if(line[0] == (char)-1) // TODO
    {
        int rc = read(session->getFD(), line, sizeof(line));
        // TODO
        UNUSED(rc);
    }

    strcpy(line, "\r\n" CLI_WELCOME_MSG "\r\n");
    NetUtil::send(session->getFD(), line, strlen(line), SEND_TIMEOUT);
    NetUtil::send(session->getFD(), CLI_PROMPT, strlen(CLI_PROMPT), SEND_TIMEOUT);
}

void CLI::onTcpSessionRemoved(TcpSession* session)
{
    TelnetSession* telnetSession = (TelnetSession*)session->getParam();
    session->setParam(NULL);

    delete telnetSession;
}

void CLI::onTcpSessionReadable(TcpSession* session)
{
    VKey_t vkey;

    TelnetSession* telnetSession = (TelnetSession*)session->getParam();

    if (ReadVKey(session->getFD(), &vkey) == 1)
    {
        /* Convert 0xD0 0x00 to RETURN */
        if (vkey.mCode == VKEY_CODE_CARRIAGE_RETURN)
        {
            telnetSession->setCR(true);
        }
        else if (vkey.mCode == VKEY_CODE_NULL)
        {
            if (telnetSession->isCR())
            {
                vkey.mCode = VKEY_CODE_RETURN;
                telnetSession->setCR(false);
            }
        }
        else
            telnetSession->setCR(false);

        telnetSession->processVKey(&vkey);

        if (vkey.mCode == VKEY_CODE_RETURN)
        {
            char** argv = NULL;
            int argc = make_args(telnetSession->getLine(), &argv);
            if(argv)
            {
                Lock lock(mLock);
                auto it = mCmdMap.find(argv[0]);
                if (it != mCmdMap.end())
                {
                    it->second->execute(telnetSession, argc, argv);
                }
                else
                {
                    telnetSession->print("%s: command not found\n", argv[0]);
                }

                free_args(argv);
            }
            telnetSession->reset();
            NetUtil::send(session->getFD(), CLI_PROMPT, strlen(CLI_PROMPT), SEND_TIMEOUT);
        }
    }
}

void CLI::addCommand(const char* command, CLI_Func fnCli, void* param, const char* desc)
{
    Lock lock(mLock);

    auto it = mCmdMap.find(command);
    if (it != mCmdMap.end())
    {
        LOGE("already has a command : %s", command);
        return;
    }

    if (desc)
        mCmdMap.insert(std::pair<std::string, std::shared_ptr<Command>>(command,
                                std::make_shared<Command>(command, param, fnCli, desc)));
    else
        mCmdMap.insert(std::pair<std::string, std::shared_ptr<Command>>(command,
                                std::make_shared<Command>(command, param, fnCli)));
}

void CLI::removeCommand(const char* command)
{
    Lock lock(mLock);

    auto it = mCmdMap.find(command);
    if (it == mCmdMap.end())
        return;

    mCmdMap.erase(it);
}

void TelnetSession::print(const char* fmt, ...)
{
    char buffer[4*1024];
    va_list ap;
    va_start(ap, fmt);
    vsprintf(buffer, fmt, ap);

    // Convert \n to \r\n
    {
        char* begin = buffer;
        char* end;

        while ((end = strchr(begin, '\n')) != NULL)
        {
            if (end == begin)
                NetUtil::send(mSession->getFD(), "\r\n", 2, SEND_TIMEOUT);
            else if (*(end - 1) == '\r')
                NetUtil::send(mSession->getFD(), begin, end - begin + 1, SEND_TIMEOUT);
            else // insert '\r'
            {
                NetUtil::send(mSession->getFD(), begin, end - begin, SEND_TIMEOUT);
                NetUtil::send(mSession->getFD(), "\r\n", 2, SEND_TIMEOUT);
            }

            begin = end + 1;
        }

        if (begin)
        {
            NetUtil::send(mSession->getFD(), begin, strlen(begin), SEND_TIMEOUT);
        }
    }
}

void TelnetSession::printHistory()
{
    print("History:\n");
    for (int ii = 0; ii < mCountHistory; ii++)
    {
        int index = (mLastHistory - ii + MAX_HISTORY_CNT) % MAX_HISTORY_CNT;
        print("[%d] %s\n", ii+1, mCmdLines[index]);
    }
}

const char* TelnetSession::searchHistory(int step)
{
    if (mCountHistory == 0)
        return NULL;

    if (step < 0)
        return NULL;

    if (mCountHistory <= step)
        return NULL;

    return mCmdLines[(mLastHistory - step + MAX_HISTORY_CNT) % MAX_HISTORY_CNT];
}

void TelnetSession::removeHistory()
{
    if (mCountHistory == 0)
        return;

    mFirstHistory = (mFirstHistory + 1) % MAX_HISTORY_CNT;
    mCountHistory --;
}

void TelnetSession::insertHistory(const char* cmd)
{
    if (mCountHistory == MAX_HISTORY_CNT)
        removeHistory();

    mLastHistory = (mLastHistory + 1) % MAX_HISTORY_CNT;
    memcpy(mCmdLines[mLastHistory], cmd, MAX_READ_LINE_LEN);
    mCmdLines[mLastHistory][MAX_READ_LINE_LEN - 1] = '\0';
    mCountHistory ++;
}

#define ERASE_CHAR  "\b \b"
void TelnetSession::doInsertChars(const char* charIn, int charInCnt)
{
    static char szBack[MAX_READ_LINE_LEN];

    int fd = mSession->getFD();

    if (charInCnt == 0)
        return;

    if (mCharCnt + charInCnt > MAX_READ_LINE_LEN)
        return;

    if (mCaretPos == mCharCnt) // End Of Line
    {
        for (int ii = 0; ii < charInCnt; ii++)
        {
            mLine[mCharCnt] = charIn[ii];
            mCharCnt++;
            mCaretPos++;
        }
        NetUtil::send(fd, charIn, charInCnt, SEND_TIMEOUT);
    }
    else // Insert Middle of Line
    {
        char* curPos    = mLine + mCaretPos;
        char* targetPos = mLine + mCaretPos + charInCnt;
        int   moveLen   = mCharCnt - mCaretPos;
        memmove(targetPos, curPos, moveLen);
        memcpy(curPos, charIn, charInCnt);

        NetUtil::send(fd, curPos, moveLen + charInCnt, SEND_TIMEOUT);

        memset(szBack, '\b', moveLen);
        NetUtil::send(fd, szBack, moveLen, SEND_TIMEOUT);

        #if 0
        for(nIdx = 0; nIdx< nMoveLen; nIdx++)
            fwrite(nSock, &cBack, 1);
        #endif
        mCaretPos += charInCnt;
        mCharCnt  += charInCnt;
    }
}

void TelnetSession::doErase(int eraseCnt)
{
    int fd = mSession->getFD();
    char szErase[MAX_READ_LINE_LEN];

    if(eraseCnt == 0)
        return;

    if(mCharCnt <= 0)
        return;

    if(mCaretPos < eraseCnt)
        return;

    if(mCaretPos < mCharCnt) // Middle of Line
    {
        char* curPos = mLine + mCaretPos;
        char* targetPos = curPos - eraseCnt;

        int moveLen = mCharCnt - mCaretPos;

        memmove(targetPos, curPos, moveLen);
        memset(targetPos + moveLen, ' ', eraseCnt);


        memset(szErase, '\b', eraseCnt);
        memcpy(szErase + eraseCnt, targetPos, moveLen + eraseCnt);
        NetUtil::send(fd, szErase, moveLen + eraseCnt + eraseCnt, SEND_TIMEOUT);
        memset(szErase, '\b', moveLen + eraseCnt);
        NetUtil::send(fd, szErase, moveLen + eraseCnt, SEND_TIMEOUT);

        mCaretPos -= eraseCnt;
        mCharCnt  -= eraseCnt;
    }
    else
    {
        for(int ii = 0; ii < eraseCnt; ii++)
            NetUtil::send(fd, ERASE_CHAR, 3, SEND_TIMEOUT);

        mCaretPos -= eraseCnt;
        mCharCnt  -= eraseCnt;
    }
}

void TelnetSession::doDelete(int deleteCnt)
{
    int fd = mSession->getFD();

    char szDelete[MAX_READ_LINE_LEN];

    char* curPos = mLine + mCaretPos;
    int   remain = mCharCnt - mCaretPos;

    if(deleteCnt == 0)
        return;

    if(remain == 0)
        return;

    if(remain < deleteCnt)
        deleteCnt = remain;

    memmove(curPos, curPos + deleteCnt, remain - deleteCnt);
    memset(curPos + remain - deleteCnt, ' ',  deleteCnt);

    memcpy(szDelete, curPos, remain);
    NetUtil::send(fd, szDelete, remain, SEND_TIMEOUT);
    memset(szDelete, '\b', remain);
    NetUtil::send(fd, szDelete, remain, SEND_TIMEOUT);

    mCharCnt -= deleteCnt;
}

#define CR "\r\n"
void TelnetSession::doReturn()
{
    int fd = mSession->getFD();

    mLine[mCharCnt] = '\0';

    NetUtil::send(fd, CR, 2, SEND_TIMEOUT);
}

void TelnetSession::doClearLine()
{
    int fd = mSession->getFD();

    memset(mLine, '\b', mCaretPos);
    NetUtil::send(fd, mLine, mCaretPos, SEND_TIMEOUT);

    memset(mLine, ' ', mCharCnt);
    NetUtil::send(fd, mLine, mCharCnt, SEND_TIMEOUT);
    memset(mLine, '\b', mCharCnt);
    NetUtil::send(fd, mLine, mCharCnt, SEND_TIMEOUT);

    memset(mLine, ' ', mCharCnt);

    mCharCnt  = 0;
    mCaretPos = 0;
}

void TelnetSession::doMoveLeft()
{
    int fd = mSession->getFD();

    char cBack = '\b';

    if(mCaretPos <= 0)
        return;

    mCaretPos--;
    NetUtil::send(fd, &cBack, 1, SEND_TIMEOUT);
}

void TelnetSession::doMoveRight()
{
    int fd = mSession->getFD();

    if(mCaretPos >= mCharCnt)
        return;

    NetUtil::send(fd, &mLine[mCaretPos], 1, SEND_TIMEOUT);
    mCaretPos++;
}

void TelnetSession::processVKey(const VKey_t* vkey)
{
    switch(vkey->mCode)
    {
        case VKEY_CODE_CHAR:
            doInsertChars(&vkey->mValue, 1);
            break;
        case VKEY_CODE_ARROW_LEFT:
            doMoveLeft();
            break;
        case VKEY_CODE_ARROW_RIGHT:
            doMoveRight();
            break;
        case VKEY_CODE_ARROW_UP:
        {
            const char* history = searchHistory(++mStepHistory);
            if (history)
            {
                doClearLine();
                doInsertChars(history, strlen(history));
            }
            else
                mStepHistory--;

            break;
        }
        case VKEY_CODE_ARROW_DOWN:
        {
            const char* history = searchHistory(--mStepHistory);
            if (history)
            {
                doClearLine();
                doInsertChars(history, strlen(history));
            }
            else
                mStepHistory++;

            break;
        }
        case VKEY_CODE_RETURN:
            doReturn();
            if (strlen(mLine) > 0)
                insertHistory(mLine);
            mStepHistory = -1;
            break;
        case VKEY_CODE_BACKSPACE:
            doErase(1);
            break;
        case VKEY_CODE_CTRL_RIGHT:
        case VKEY_CODE_END_OF_LINE:
            break;
        case VKEY_CODE_CTRL_LEFT:
        case VKEY_CODE_BEGIN_OF_LINE:
            break;
        case VKEY_CODE_DELETE:
            doDelete(1);
            break;
        case VKEY_CODE_TAB:
            break;
        case VKEY_CODE_CLEAR_LINE:
            break;
        default:
            break;
    }
}

void TelnetSession::reset()
{
    mLine[0]   = 0;
    mCharCnt   = 0;
    mCaretPos = 0;
}

void CLI::cmd_clear(ICliSession* session, int argc, char** argv, void* param)
{
//    static const char* clr = "\x1[2j";
    static const char* clr = "\33[H\33[2J";
    UNUSED(param);
    UNUSED(argc);
    UNUSED(argv);
    CLI_OUT(clr);
}

void CLI::cmd_help(ICliSession* session, int argc, char** argv, void* param)
{
    UNUSED(param);
    UNUSED(argc);
    UNUSED(argv);
    CLI_OUT("Commands :\n");

    CLI* pThis = (CLI*)param;

    std::map<std::string, std::shared_ptr<Command>>::iterator it;
    for (it = pThis->mCmdMap.begin(); it != pThis->mCmdMap.end(); it++)
    {
        CLI_OUT("  %-30s %s\n", it->second->cmd().c_str(), it->second->desc().c_str());
    }
}

void CLI::cmd_history(ICliSession* session, int argc, char** argv, void* param)
{
    TelnetSession* telnetSession = (TelnetSession*)session;
    UNUSED(param);
    UNUSED(argc);
    UNUSED(argv);

    telnetSession->printHistory();
}

