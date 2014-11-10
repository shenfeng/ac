#include <boost/program_options.hpp>
#include <thrift/concurrency/ThreadManager.h>
// #include <thrift/concurrency/PosixThreadFactory.h>
//#include <thrift/protocol/TBinaryProtocol.h>
// #include <thrift/protocol/TCompactProtocol.h>
// #include <thrift/server/TNonblockingServer.h>
#include <thrift/server/TThreadPoolServer.h>
// #include <thrift/server/TThreadPoolServer.h>
// #include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TServerSocket.h>
// #include <thrift/transport/TTransportUtils.h>
#include <concurrency/PlatformThreadFactory.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TBufferTransports.h> // TFramedTransportFactory

#include "AutoComplete.h"
#include <vector>
#include <string>
#include <iostream>
#include "acindex.h"
#include "logger.hpp"

#include <thread>
#include <mutex>


using namespace std;

struct DataServer {
    int verbosity;
    int mode;

    int threads;  // how many thrift worker threads
    int port;                   //  which port to listen to

    std::string pid_port;
    std::string data_dir; // data dir
    std::string pinyin_file; // data dir
};

// easier to code
static struct DataServer confs;       // global
// static Indexes indexes;
std::unordered_map<std::string, AcIndex *> indexes;
static Pinyins pinyins;

class AutoCompleteHandler : virtual public AutoCompleteIf {
private:
public:
    void autocomplete(AcResp &_return, const AcReq &req) {
        auto it = indexes.find(req.kind);
        if (it != indexes.end()) {
            AcRequest q(req.q, req.limit, req.offset);
            AcResult r;
            it->second->Search(q, r);
            _return.total = r.hits;
            for (auto it = r.items.begin(); it != r.items.end(); it++) {
                AcRespItem item;
                item.item = it->data;
                item.score = it->score;
                item.__set_highlighted(it->highlighted);
                _return.items.push_back(item);
            }
        }
    }

    void acnames(std::vector<std::string> &_return) {
        for (auto it = indexes.cbegin(); it != indexes.cend(); ++it) {
            _return.push_back(it->first);
        }
    }
};

void start_thrift_server(AutoCompleteHandler &h) {
    using boost::shared_ptr;
    using namespace apache::thrift;
    using namespace apache::thrift::protocol;
    using namespace apache::thrift::transport;
    using namespace apache::thrift::server;
    using namespace apache::thrift::concurrency;

    shared_ptr<AutoCompleteHandler> handler(&h);
    shared_ptr<TProcessor> processor(new AutoCompleteProcessor(handler));
    shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());
    shared_ptr<TServerTransport> serverTransport(new TServerSocket(confs.port));
    shared_ptr<TTransportFactory> transportFactory(new TFramedTransportFactory());

    shared_ptr<ThreadManager> threadManager =
            ThreadManager::newSimpleThreadManager(confs.threads);
    shared_ptr<PlatformThreadFactory> threadFactory =
            shared_ptr<PlatformThreadFactory>(new PlatformThreadFactory());
    threadManager->threadFactory(threadFactory);
    threadManager->start();

    TThreadPoolServer s(processor, serverTransport, transportFactory, protocolFactory,
            threadManager);
    log_info("thrift server started, port: %d, threads: %d",
            confs.port, confs.threads);

    s.serve();
}

template<class T>
class ConcurrentList {
private:
    list<T> mList;
    std::mutex mMutex;
public:
    ConcurrentList(const std::vector<T> &v) : mList(v.begin(), v.end()) {
    }

    std::pair<T, bool> pop_back() {
        lock_guard<mutex> _(mMutex);
        if (!mList.empty()) {
            auto p = mList.back();
            mList.pop_back();
            return std::make_pair(p, true);
        }
        return std::make_pair(T(), false);
    }
};

class Runner {
private:
    ConcurrentList<string> &mList;
    std::mutex &mMutex;
    int mJobId;
public:
    Runner(ConcurrentList<string> &l, std::mutex &mutex, int jobid) :
            mList(l), mMutex(mutex), mJobId(jobid) {
    }

    int operator()() {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "worker-%d", mJobId);
        set_thread_name(buffer);
        int count = 0;
        while (1) {
            auto p = mList.pop_back();
            if (p.second) {
                count += 1;
                AcIndex *index = new AcIndex(pinyins);
                if (index->Open(confs.data_dir + "/" + p.first) >= 0) {
                    lock_guard<mutex> _(mMutex);
                    indexes[index->GetName()] = index;
                    // indexes.Add(index);
                }
            } else {
                break;
            }
        }
        log_info("finish job: %d, run %d jobs", mJobId, count);
        return 0;
    }
};


void parse_args(char **argv, int argc) {
    namespace po = boost::program_options;
    using boost::program_options::value;


    auto &conf = confs;

    po::options_description desc("Auto complete server, allowed options", 200);
    string logfile;
    desc.add_options()
            ("help,h", "Display a help message and exit.")
            ("loglevel", value<int>(&conf.verbosity)->default_value(2),
                    "Can be 0(debug), 1(verbose), 2(notice), 3(warnning)")
            ("logfile", value<string>(&logfile)->default_value("stdout"),
                    "Log file, can be stdout")
            ("dir", value<string>(&conf.data_dir)->default_value("bdata"),
                    "data dir to build index")
            ("pinyin", value<string>(&conf.pinyin_file)->default_value("pinyins"),
                    "hanzhi <=> pinyin mapping file")
            ("threads,t", value<int>(&conf.threads)->default_value(8),
                    "Threads count")
            ("port,p", value<int>(&conf.port)->default_value(8912),
                    "Port to listen for thrift connection");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        exit(1);
    }
    set_thread_name("main");
    log_open(logfile, conf.verbosity, 24);
    log_info("log to file: %s, verbosity: %d", logfile.data(), conf.verbosity);
}

int main(int argc, char **argv) {
    parse_args(argv, argc);

    if (pinyins.Open(confs.pinyin_file) <= 0) {
        log_fatal("open pinyin failed: %s", confs.pinyin_file.data());
        return 1;
    }

    {
        std::vector<std::string> files;
        DIR *d = opendir(confs.data_dir.data());
        if (d == NULL) {
            perror(confs.data_dir.data());
            return -1;
        }
        struct dirent *dp;
        while ((dp = readdir(d)) != NULL) {
            if (dp->d_name[0] == '.') {
                continue;
            }
            files.push_back(dp->d_name);
        }
        closedir(d);

        std::vector<std::thread> threads;
        std::mutex lock;
        ConcurrentList<std::string> jobs(files);

        size_t cpu = thread::hardware_concurrency();
        if (cpu <= 0) {
            cpu = 2;
        }

        for (size_t i = 0; i < cpu; i++) {
            Runner r(jobs, lock, i);
            threads.push_back(std::thread(r));
        }
        for (auto f = threads.begin(); f != threads.end(); ++f) {
            f->join(); // block wait all to finish
        }
        log_info("load index using %d threads, %d files, %d indexes", cpu, files.size(), indexes.size());
    }

    AutoCompleteHandler h;
    start_thrift_server(h);
}
