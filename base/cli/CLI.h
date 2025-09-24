/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __CLI_H_
#define __CLI_H_

#include <string>
#include <map>
#include <memory>
#include <termios.h>

#include "TcpServer.h"
#include "Mutex.h"

#include "vkey.h"

#define CLI_SERVER_PORT  5555

typedef void (*CLI_Func)(void* session, int argc, char** argv, void* param);

#define CLI_OUT(fmt, args...)    do { \
                                    TelnetSession* ts = (TelnetSession*)session; \
                                    ts->print(fmt, ##args); \
                                 } while (0)

#define MAX_READ_LINE_LEN (2*1024)
#define MAX_HISTORY_CNT   16

class TelnetSession
{
public:
    TelnetSession(TcpSession* session) : mSession(session) { };
    ~TelnetSession() { };

    void stop();

    void print(const char* fmt, ...);
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


class CLI : public ITcpSessionListener
{
public:
    static CLI& getInstance();
    virtual ~CLI();

    bool start();
    void stop();

    void addCommand(const char* command, CLI_Func fnCli, void* param, const char* desc = NULL);
    void removeCommand(const char* command);

private:
    CLI();

    void onTcpSessionEstablished(TcpSession* session);
    void onTcpSessionRemoved(TcpSession* session);
    void onTcpSessionReadable(TcpSession* session);

private:
    class Command
    {
        public:
            Command(const char* command, void* param, CLI_Func func) :
                mCommand(command),
                mParam(param),
                mFunc(func),
                mDesc("") {}

            Command(const char* command, void* param, CLI_Func func, const char* desc) :
                mCommand(command),
                mParam(param),
                mFunc(func),
                mDesc(desc) {}

            const std::string& cmd() { return mCommand; }
            const std::string& desc() { return mDesc; }

            void execute(TelnetSession* session, int argc, char** argv)
            {
                if(mFunc)
                    mFunc(session, argc, argv, mParam);
            }

        private:
            std::string mCommand;
            void*       mParam;
            CLI_Func    mFunc;
            std::string mDesc;
    };

    RecursiveMutex mLock;

    std::map<std::string, std::shared_ptr<Command>> mCmdMap;

    static void cmd_clear(void* session, int argc, char** argv, void* param);
    static void cmd_help(void* session, int argc, char** argv, void* param);
    static void cmd_history(void* session, int argc, char** argv, void* param);
    static void cmd_exit(void* session, int argc, char** argv, void* param);

    TcpServer mServer;
};


#endif //__GPIO_H_
