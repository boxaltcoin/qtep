#include <qtum/qtumledger.h>
#include <util/system.h>
#include <chainparams.h>
#include <univalue.h>
#include <boost/process.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>

namespace QtumLedger_NS {
// Read json document
UniValue json_read_doc(const std::string& jsondata)
{
    UniValue v;
    v.read(jsondata);
    return v;
}

// Read json object
UniValue json_get_object(const UniValue& jsondata)
{
    UniValue v(UniValue::VOBJ);
    if(jsondata.isObject())
        v = jsondata.get_obj();
    return v;
}

// Get json string for key
std::string json_get_key_string(const UniValue& jsondata, std::string key)
{
    UniValue v(UniValue::VSTR);
    if(jsondata.exists(key))
    {
        UniValue data = jsondata[key];
        if(data.isStr())
            v = data;
    }

    return v.get_str();
}

// Append data to vector
std::vector<std::string>& operator<<(std::vector<std::string>& os, const std::string& dt)
{
    os.push_back(dt);
    return os;
}

// Start process from qtumd
class CProcess
{
public:
    ~CProcess()
    {
        clean();
    }

    // Set process params
    void start(const std::string& prog, const std::vector<std::string> &arg)
    {
        clean();
        m_program = prog;
        m_arguments = arg;
    }

    // Start and wait for it to finish
    void waitForFinished()
    {
        boost::asio::io_service svc;
        boost::asio::streambuf out, err;
        boost::process::child child(m_program, boost::process::args(m_arguments),
                                    boost::process::std_out > out, boost::process::std_err > err, svc);

        svc.run();
        child.wait();
        m_std_out = toString(&out);
        m_std_err = toString(&err);
    }

    // Read all standard output
    std::string readAllStandardOutput()
    {
        return m_std_out;
    }

    // Read all standard error
    std::string readAllStandardError()
    {
        return m_std_err;
    }

    // Clean process
    void clean()
    {
        m_program = "";
        m_std_out = "";
        m_std_err = "";
    }


private:
    std::string toString(boost::asio::streambuf* stream)
    {
        std::istream is(stream);
        std::ostringstream os;
        is >> os.rdbuf();
        return os.str();
    }

private:
    std::string m_program;
    std::vector<std::string> m_arguments;
    std::string m_std_out;
    std::string m_std_err;
};
}
using namespace QtumLedger_NS;

class QtumLedgerPriv
{
public:
    QtumLedgerPriv()
    {
        toolPath = gArgs.GetArg("-hwitoolpath", "");
        toolExists = boost::filesystem::exists(toolPath);
        if(gArgs.GetChainName() != CBaseChainParams::MAIN)
        {
            arguments << "--testnet";
        }
    }

    std::atomic<bool> fStarted{false};
    CProcess process;
    std::string strStdout;
    std::string strError;
    std::string toolPath;
    std::vector<std::string> arguments;
    bool toolExists = false;
};

QtumLedger::QtumLedger():
    d(0)
{
    d = new QtumLedgerPriv();
}

QtumLedger::~QtumLedger()
{
    if(d)
        delete d;
    d = 0;
}

bool QtumLedger::signCoinStake(const std::string &fingerprint, std::string &psbt)
{
    // Check if tool exists
    if(!toolExists())
        return false;

    // Sign PSBT transaction
    if(isStarted())
        return false;

    if(!beginSignTx(fingerprint, psbt))
        return false;

    wait();

    return endSignTx(fingerprint, psbt);
}

bool QtumLedger::signBlockHeader(const std::string &fingerprint, const std::string &header, std::vector<unsigned char> &vchSig)
{
    return false;
}

bool QtumLedger::toolExists()
{
    return d->toolExists;
}

bool QtumLedger::isStarted()
{
    return d->fStarted;
}

void QtumLedger::wait()
{
    if(d->fStarted)
    {
        d->process.waitForFinished();
        d->strStdout = d->process.readAllStandardOutput();
        d->strError = d->process.readAllStandardError();
        d->fStarted = false;
    }
}

bool QtumLedger::beginSignTx(const std::string &fingerprint, std::string &psbt)
{
    // Execute command line
    std::vector<std::string> arguments = d->arguments;
    arguments << "-f" << fingerprint << "signtx" << psbt;
    d->process.start(d->toolPath, arguments);
    d->fStarted = true;

    return d->fStarted;
}

bool QtumLedger::endSignTx(const std::string &, std::string &psbt)
{
    // Decode command line results
    UniValue jsonDocument = json_read_doc(d->strStdout);
    UniValue data = json_get_object(jsonDocument);
    std::string psbtSigned = json_get_key_string(data, "psbt");
    if(!psbtSigned.empty())
    {
        psbt = psbtSigned;
        return true;
    }

    return false;
}