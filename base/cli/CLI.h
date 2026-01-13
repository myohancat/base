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

#define MAX_READ_LINE_LEN (2*1024)
#define MAX_HISTORY_CNT   16

class ICliSession
{
public:
    virtual ~ICliSession() { }

    virtual void print(const char* fmt, ...) = 0;
};

typedef void (*CLI_Func)(ICliSession* session, int argc, char** argv, void* param);

#define CLI_OUT(fmt, args...)    do { \
                                    session->print(fmt, ##args); \
                                 } while (0)

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

            void execute(ICliSession* session, int argc, char** argv)
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

    static void cmd_clear(ICliSession* session, int argc, char** argv, void* param);
    static void cmd_help(ICliSession* session, int argc, char** argv, void* param);
    static void cmd_history(ICliSession* session, int argc, char** argv, void* param);
    static void cmd_exit(ICliSession* session, int argc, char** argv, void* param);

    TcpServer mServer;
};


#endif // __CLI_H_
