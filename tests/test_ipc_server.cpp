#include "keyinjectord/FsSecurityChecker.h"
#include "keyinjectord/KeyboardMacroInjector.h"
#include "keyinjectord/ProcfsUtils.h"
#include "keyinjectord/SocketCredentials.h"
#include "keyinjectord/device_interface.h"
#include "keyinjectord/ipc_server.h"
#include "keyinjectord/launcher_auth.h"
#include "keyinjectord/protocol.h"

#include <QCoreApplication>
#include <QLocalSocket>
#include <QSignalSpy>
#include <QTest>

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>

#include <linux/input-event-codes.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace std::chrono_literals;

class MockDevice : public keyinjectord::IDevice {
public:
    bool sendCtrlShiftV() override {
        ++ctrlShiftVCalledCount;
        return true;
    }

    std::atomic<int> ctrlShiftVCalledCount {0};
};

struct RecordedRawEvent {
    int type = 0;
    int code = 0;
    int value = 0;
};

class MockRawDevice : public keyinjectord::IRawDevice {
public:
    bool emitEvent(int type, int code, int value) override {
        events.push_back({type, code, value});
        return !shouldFail;
    }

    std::vector<RecordedRawEvent> events;
    bool shouldFail = false;
};

struct ServerRunner {
    keyinjectord::IpcServer& server;
    std::thread thread;

    explicit ServerRunner(keyinjectord::IpcServer& s)
        : server(s)
        , thread([&s]() { s.run(); }) { }

    ~ServerRunner() {
        server.stop();
        if (thread.joinable()) {
            thread.join();
        }
    }
};

class TestIpcServer : public QObject {
    Q_OBJECT

private slots:
    void testValidPasteCommand() {
        int sv[2];
        QCOMPARE(::socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sv), 0);

        MockDevice mockDevice;
        keyinjectord::IpcServer server(sv[0], mockDevice);
        ServerRunner runner(server);

        QLocalSocket client;
        QVERIFY(client.setSocketDescriptor(sv[1], QLocalSocket::ConnectedState, QIODevice::ReadWrite));

        const char cmdByte = static_cast<char>(keyinjectord::Opcode::Paste);
        client.write(&cmdByte, 1);
        client.flush();

        QVERIFY(client.waitForReadyRead(2000));
        QByteArray response = client.readAll();
        QCOMPARE(response.size(), 1);
        QCOMPARE(static_cast<uint8_t>(response[0]), static_cast<uint8_t>(keyinjectord::ResponseStatus::Ok));

        QCOMPARE(mockDevice.ctrlShiftVCalledCount.load(), 1);

        client.close();
    }

    void testPingCommand() {
        int sv[2];
        QCOMPARE(::socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sv), 0);

        MockDevice mockDevice;
        keyinjectord::IpcServer server(sv[0], mockDevice);
        ServerRunner runner(server);

        QLocalSocket client;
        QVERIFY(client.setSocketDescriptor(sv[1], QLocalSocket::ConnectedState, QIODevice::ReadWrite));

        const char cmdByte = static_cast<char>(keyinjectord::Opcode::Ping);
        client.write(&cmdByte, 1);
        client.flush();

        QVERIFY(client.waitForReadyRead(2000));
        QByteArray response = client.readAll();
        QCOMPARE(response.size(), 1);
        QCOMPARE(static_cast<uint8_t>(response[0]), static_cast<uint8_t>(keyinjectord::ResponseStatus::Ok));

        QCOMPARE(mockDevice.ctrlShiftVCalledCount.load(), 0);

        client.close();
    }

    void testDeviceErrorResponse() {
        int sv[2];
        QCOMPARE(::socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sv), 0);

        class FailingMockDevice : public keyinjectord::IDevice {
        public:
            bool sendCtrlShiftV() override { return false; }
        } failingDevice;

        keyinjectord::IpcServer server(sv[0], failingDevice);
        ServerRunner runner(server);

        QLocalSocket client;
        QVERIFY(client.setSocketDescriptor(sv[1], QLocalSocket::ConnectedState, QIODevice::ReadWrite));

        const char cmdByte = static_cast<char>(keyinjectord::Opcode::Paste);
        client.write(&cmdByte, 1);
        client.flush();

        QVERIFY(client.waitForReadyRead(2000));
        QByteArray response = client.readAll();
        QCOMPARE(response.size(), 1);
        QCOMPARE(static_cast<uint8_t>(response[0]), static_cast<uint8_t>(keyinjectord::ResponseStatus::DeviceError));

        client.close();
    }

    void testUnknownOpcodeDisconnect() {
        int sv[2];
        QCOMPARE(::socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sv), 0);

        MockDevice mockDevice;
        keyinjectord::IpcServer server(sv[0], mockDevice);
        ServerRunner runner(server);

        QLocalSocket client;
        QVERIFY(client.setSocketDescriptor(sv[1], QLocalSocket::ConnectedState, QIODevice::ReadWrite));

        const char invalidCmd = static_cast<char>(0xFF);
        client.write(&invalidCmd, 1);
        client.flush();

        QVERIFY(client.waitForReadyRead(2000));
        QByteArray response = client.readAll();
        QCOMPARE(response.size(), 1);
        QCOMPARE(static_cast<uint8_t>(response[0]), static_cast<uint8_t>(keyinjectord::ResponseStatus::UnknownCmd));

        QVERIFY(client.waitForDisconnected(2000) || client.state() == QLocalSocket::UnconnectedState);
        QCOMPARE(mockDevice.ctrlShiftVCalledCount.load(), 0);
    }

    void testPeerDisconnectShutsDownServer() {
        int sv[2];
        QCOMPARE(::socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sv), 0);

        MockDevice mockDevice;
        keyinjectord::IpcServer server(sv[0], mockDevice);

        std::thread serverThread([&server]() { server.run(); });

        ::close(sv[1]);

        serverThread.join();
        QVERIFY(true);
    }

    void testInvalidDescriptorThrows() {
        MockDevice mockDevice;
        QVERIFY_EXCEPTION_THROWN(keyinjectord::IpcServer server(-1, mockDevice), std::invalid_argument);
    }

    void testAuthorizeLauncherSuccess() {
        int sv[2];
        QCOMPARE(::socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sv), 0);

        keyinjectord::AuthResult res = keyinjectord::AuthResult::InvalidFd;
        bool ok = keyinjectord::authorizeLauncher(sv[0], &res);
        ::close(sv[0]);
        ::close(sv[1]);

        QCOMPARE(res, keyinjectord::AuthResult::Success);
        QVERIFY(ok);
    }

    void testAuthorizeLauncherInvalidFd() {
        keyinjectord::AuthResult res = keyinjectord::AuthResult::Success;
        bool ok = keyinjectord::authorizeLauncher(-1, &res);
        QCOMPARE(res, keyinjectord::AuthResult::InvalidFd);
        QVERIFY(!ok);
    }

    void testAuthorizeLauncherNonSocketFd() {
        keyinjectord::AuthResult res = keyinjectord::AuthResult::Success;
        bool ok = keyinjectord::authorizeLauncher(1, &res);
        QCOMPARE(res, keyinjectord::AuthResult::NotASocket);
        QVERIFY(!ok);
    }

    std::filesystem::path currentExePath() const {
        std::array<char, PATH_MAX> selfExeBuf{};
        ssize_t len = readlink("/proc/self/exe", selfExeBuf.data(), selfExeBuf.size() - 1);
        if (len <= 0) return {};
        selfExeBuf[len] = '\0';
        return std::filesystem::path(selfExeBuf.data());
    }

    void testValidateExecutableTopologyRejectsUntrustedPrefixes() {
        const std::filesystem::path selfPath = currentExePath();
        QVERIFY(!selfPath.empty());

        keyinjectord::AuthResult res = keyinjectord::AuthResult::Success;

        QVERIFY(!keyinjectord::validateExecutableTopology("/tmp/qtscribe", selfPath, &res));
        QCOMPARE(res, keyinjectord::AuthResult::UntrustedLocation);

        QVERIFY(!keyinjectord::validateExecutableTopology("/var/tmp/qtscribe", selfPath, &res));
        QCOMPARE(res, keyinjectord::AuthResult::UntrustedLocation);

        QVERIFY(!keyinjectord::validateExecutableTopology("/dev/shm/qtscribe", selfPath, &res));
        QCOMPARE(res, keyinjectord::AuthResult::UntrustedLocation);

        QVERIFY(!keyinjectord::validateExecutableTopology("/run/user/1000/qtscribe", selfPath, &res));
        QCOMPARE(res, keyinjectord::AuthResult::UntrustedLocation);
    }

    void testValidateExecutableTopologyRejectsDeletedExecutable() {
        const std::filesystem::path selfPath = currentExePath();
        QVERIFY(!selfPath.empty());

        keyinjectord::AuthResult res = keyinjectord::AuthResult::Success;
        std::filesystem::path deletedParent = "/usr/bin/qtscribe (deleted)";
        QVERIFY(!keyinjectord::validateExecutableTopology(deletedParent, selfPath, &res));
        QCOMPARE(res, keyinjectord::AuthResult::DeletedExecutable);

        std::filesystem::path deletedSelf = selfPath.string() + " (deleted)";
        QVERIFY(!keyinjectord::validateExecutableTopology(selfPath, deletedSelf, &res));
        QCOMPARE(res, keyinjectord::AuthResult::DeletedExecutable);
    }

    void testValidateExecutableTopologyRejectsUnauthorizedName() {
        const std::filesystem::path selfPath = currentExePath();
        QVERIFY(!selfPath.empty());

        keyinjectord::AuthResult res = keyinjectord::AuthResult::Success;
        std::filesystem::path badName = selfPath.parent_path() / "unauthorized_binary";
        QVERIFY(!keyinjectord::validateExecutableTopology(badName, selfPath, &res));
        QCOMPARE(res, keyinjectord::AuthResult::UnauthorizedExecutable);
    }

    void testValidateExecutableTopologyAcceptsSelfInvocation() {
        const std::filesystem::path selfPath = currentExePath();
        QVERIFY(!selfPath.empty());

        keyinjectord::AuthResult res = keyinjectord::AuthResult::InvalidFd;
        QVERIFY(keyinjectord::validateExecutableTopology(selfPath, selfPath, &res));
        QCOMPARE(res, keyinjectord::AuthResult::Success);
    }

    void testValidateExecutableTopologyRejectsNonColocated() {
        const std::filesystem::path selfPath = currentExePath();
        QVERIFY(!selfPath.empty());

        std::filesystem::path nestedDir = selfPath.parent_path() / "test_nested_dir";
        std::filesystem::create_directories(nestedDir);
        std::filesystem::path nestedBinary = nestedDir / "qtscribe";

        FILE* f = fopen(nestedBinary.c_str(), "w");
        QVERIFY(f != nullptr);
        fclose(f);

        keyinjectord::AuthResult res = keyinjectord::AuthResult::Success;

        bool ok = keyinjectord::validateExecutableTopology(nestedBinary, selfPath, &res);
        QVERIFY(!ok);
        QCOMPARE(res, keyinjectord::AuthResult::UntrustedLocation);

        std::filesystem::remove(nestedBinary);
        std::filesystem::remove(nestedDir);
    }

    void testValidateExecutableTopologyRejectsWorldWritable() {
        const std::filesystem::path selfPath = currentExePath();
        QVERIFY(!selfPath.empty());

        std::filesystem::path testDir = selfPath.parent_path() / "test_isolated_world_writable_dir";
        std::filesystem::create_directories(testDir);

        std::filesystem::path mockSelf = testDir / "test_ipc_server";
        FILE* fSelf = fopen(mockSelf.c_str(), "w");
        QVERIFY(fSelf != nullptr);
        fclose(fSelf);

        std::filesystem::path mockParent = testDir / "qtscribe";
        FILE* fParent = fopen(mockParent.c_str(), "w");
        QVERIFY(fParent != nullptr);
        fclose(fParent);

        QCOMPARE(::chmod(mockParent.c_str(), 0777), 0);

        keyinjectord::AuthResult res = keyinjectord::AuthResult::Success;
        bool ok = keyinjectord::validateExecutableTopology(mockParent, mockSelf, &res);
        QVERIFY(!ok);
        QCOMPARE(res, keyinjectord::AuthResult::WorldWritable);

        std::error_code ec;
        std::filesystem::remove_all(testDir, ec);
    }

    void testValidateExecutableTopologyRejectsGroupWritableInProduction() {
        const std::filesystem::path selfPath = currentExePath();
        QVERIFY(!selfPath.empty());

        std::filesystem::path testDir = selfPath.parent_path() / "test_isolated_group_writable_dir";
        std::filesystem::create_directories(testDir);

        std::filesystem::path mockSelf = testDir / "test_ipc_server";
        FILE* fSelf = fopen(mockSelf.c_str(), "w");
        QVERIFY(fSelf != nullptr);
        fclose(fSelf);

        std::filesystem::path mockParent = testDir / "qtscribe";
        FILE* fParent = fopen(mockParent.c_str(), "w");
        QVERIFY(fParent != nullptr);
        fclose(fParent);

        QCOMPARE(::chmod(mockParent.c_str(), 0775), 0);

        keyinjectord::AuthResult res = keyinjectord::AuthResult::Success;
        bool ok = keyinjectord::validateExecutableTopology(mockParent, mockSelf, &res, /*allowDevMode=*/true,
                                                           /*outParentStat=*/nullptr, /*rejectGroupWritable=*/true);
        QVERIFY(!ok);
        QCOMPARE(res, keyinjectord::AuthResult::GroupWritable);

        std::error_code ec;
        std::filesystem::remove_all(testDir, ec);
    }

    void testValidateExecutableTopologyRejectsNonRootOwnerInProduction() {
        const std::filesystem::path selfPath = currentExePath();
        QVERIFY(!selfPath.empty());

        std::filesystem::path testDir = selfPath.parent_path() / "test_isolated_non_root_dir";
        std::filesystem::create_directories(testDir);

        std::filesystem::path mockSelf = testDir / "test_ipc_server";
        FILE* fSelf = fopen(mockSelf.c_str(), "w");
        QVERIFY(fSelf != nullptr);
        fclose(fSelf);

        std::filesystem::path mockParent = testDir / "qtscribe";
        FILE* fParent = fopen(mockParent.c_str(), "w");
        QVERIFY(fParent != nullptr);
        fclose(fParent);

        ::chmod(mockParent.c_str(), 0755);
        ::chmod(mockSelf.c_str(), 0755);
        ::chmod(testDir.c_str(), 0755);

        if (::getuid() == 0) {
            keyinjectord::AuthResult rootRes = keyinjectord::AuthResult::InvalidFd;
            bool rootOk =
                keyinjectord::validateExecutableTopology(mockParent, mockSelf, &rootRes, /*allowDevMode=*/false);
            QVERIFY(rootOk);
            QCOMPARE(rootRes, keyinjectord::AuthResult::Success);

            if (::chown(mockParent.c_str(), 1000, 1000) == 0) {
                keyinjectord::AuthResult nonRootRes = keyinjectord::AuthResult::Success;
                bool nonRootOk =
                    keyinjectord::validateExecutableTopology(mockParent, mockSelf, &nonRootRes, /*allowDevMode=*/false);
                QVERIFY(!nonRootOk);
                QCOMPARE(nonRootRes, keyinjectord::AuthResult::NonRootOwner);
            }
        } else {
            keyinjectord::AuthResult res = keyinjectord::AuthResult::Success;
            bool ok = keyinjectord::validateExecutableTopology(mockParent, mockSelf, &res, /*allowDevMode=*/false);
            QVERIFY(!ok);
            QCOMPARE(res, keyinjectord::AuthResult::NonRootOwner);
        }

        std::error_code ec;
        std::filesystem::remove_all(testDir, ec);
    }

    void testValidateExecutableTopologyRejectsWritableGrandparent() {
        const std::filesystem::path selfPath = currentExePath();
        QVERIFY(!selfPath.empty());

        std::filesystem::path grandparentDir = selfPath.parent_path() / "test_grandparent_dir";
        std::filesystem::path parentDir = grandparentDir / "parent_dir";
        std::filesystem::create_directories(parentDir);

        std::filesystem::path mockSelf = parentDir / "test_ipc_server";
        FILE* fSelf = fopen(mockSelf.c_str(), "w");
        QVERIFY(fSelf != nullptr);
        fclose(fSelf);

        std::filesystem::path mockParent = parentDir / "qtscribe";
        FILE* fParent = fopen(mockParent.c_str(), "w");
        QVERIFY(fParent != nullptr);
        fclose(fParent);

        QCOMPARE(::chmod(grandparentDir.c_str(), 0777), 0);

        keyinjectord::AuthResult res = keyinjectord::AuthResult::Success;
        bool ok = keyinjectord::validateExecutableTopology(mockParent, mockSelf, &res, /*allowDevMode=*/true);
        QVERIFY(!ok);
        QCOMPARE(res, keyinjectord::AuthResult::WorldWritable);

        QCOMPARE(::chmod(grandparentDir.c_str(), 0775), 0);
        res = keyinjectord::AuthResult::Success;
        ok = keyinjectord::validateExecutableTopology(mockParent, mockSelf, &res, /*allowDevMode=*/true,
                                                      /*outParentStat=*/nullptr, /*rejectGroupWritable=*/true);
        QVERIFY(!ok);
        QCOMPARE(res, keyinjectord::AuthResult::GroupWritable);

        std::error_code ec;
        std::filesystem::remove_all(grandparentDir, ec);
    }

    void testValidateDirectoryAncestryDirect() {
        const std::filesystem::path selfPath = currentExePath();
        QVERIFY(!selfPath.empty());

        keyinjectord::AuthResult res = keyinjectord::AuthResult::InvalidFd;
        struct stat leafStat {};
        bool ok = keyinjectord::validateDirectoryAncestry(selfPath, &res, /*allowDevMode=*/true, &leafStat);
        QVERIFY(ok);
        QCOMPARE(res, keyinjectord::AuthResult::Success);
        QVERIFY(S_ISREG(leafStat.st_mode));
        QVERIFY(leafStat.st_nlink > 0);

        res = keyinjectord::AuthResult::Success;
        QVERIFY(!keyinjectord::validateDirectoryAncestry("/tmp", &res, /*allowDevMode=*/true));
        QCOMPARE(res, keyinjectord::AuthResult::UntrustedLocation);

        res = keyinjectord::AuthResult::Success;
        QVERIFY(!keyinjectord::validateDirectoryAncestry("/nonexistent_path_xyz_123", &res, /*allowDevMode=*/true));
        QCOMPARE(res, keyinjectord::AuthResult::StatFailed);
    }

    void testAuthResultToString() {
        QCOMPARE(QString(keyinjectord::authResultToString(keyinjectord::AuthResult::Success)), QString("Success"));
        QCOMPARE(QString(keyinjectord::authResultToString(keyinjectord::AuthResult::WorldWritable)),
                 QString("Parent executable or ancestor directory is world-writable"));
        QCOMPARE(QString(keyinjectord::authResultToString(keyinjectord::AuthResult::GroupWritable)),
                 QString("Parent executable or ancestor directory is group-writable"));
        QCOMPARE(QString(keyinjectord::authResultToString(keyinjectord::AuthResult::NonRootOwner)),
                 QString("Production helper executable, parent binary, or directory is not owned by root (UID 0)"));
        QCOMPARE(QString(keyinjectord::authResultToString(keyinjectord::AuthResult::InodeMismatch)),
                 QString("Process executable inode does not match filesystem path inode"));
    }

    void testHandleOpcodeIsolated() {
        int sv[2];
        QCOMPARE(::socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sv), 0);

        MockDevice mockDevice;
        keyinjectord::IpcServer server(sv[0], mockDevice);

        QCOMPARE(server.handleOpcode(keyinjectord::Opcode::Paste), keyinjectord::ResponseStatus::Ok);
        QCOMPARE(mockDevice.ctrlShiftVCalledCount.load(), 1);

        QCOMPARE(server.handleOpcode(keyinjectord::Opcode::Ping), keyinjectord::ResponseStatus::Ok);
        QCOMPARE(mockDevice.ctrlShiftVCalledCount.load(), 1);

        QCOMPARE(server.handleOpcode(static_cast<keyinjectord::Opcode>(0xFE)),
                 keyinjectord::ResponseStatus::UnknownCmd);

        class FailingDevice : public keyinjectord::IDevice {
        public:
            bool sendCtrlShiftV() override { return false; }
        } failingDevice;

        keyinjectord::IpcServer failingServer(sv[1], failingDevice);
        QCOMPARE(failingServer.handleOpcode(keyinjectord::Opcode::Paste),
                 keyinjectord::ResponseStatus::DeviceError);

        server.stop();
        failingServer.stop();
    }

    void testKeyboardMacroInjectorSequence() {
        MockRawDevice mockRaw;
        keyinjectord::KeyboardMacroInjector injector(mockRaw, 0ms);

        QVERIFY(injector.sendCtrlShiftV());

        QCOMPARE(mockRaw.events.size(), 12);

        QCOMPARE(mockRaw.events[0].type, EV_KEY);
        QCOMPARE(mockRaw.events[0].code, KEY_LEFTCTRL);
        QCOMPARE(mockRaw.events[0].value, 1);

        QCOMPARE(mockRaw.events[1].type, EV_SYN);
        QCOMPARE(mockRaw.events[1].code, SYN_REPORT);
        QCOMPARE(mockRaw.events[1].value, 0);

        QCOMPARE(mockRaw.events[2].type, EV_KEY);
        QCOMPARE(mockRaw.events[2].code, KEY_LEFTSHIFT);
        QCOMPARE(mockRaw.events[2].value, 1);

        QCOMPARE(mockRaw.events[3].type, EV_SYN);
        QCOMPARE(mockRaw.events[3].code, SYN_REPORT);
        QCOMPARE(mockRaw.events[3].value, 0);

        QCOMPARE(mockRaw.events[4].type, EV_KEY);
        QCOMPARE(mockRaw.events[4].code, KEY_V);
        QCOMPARE(mockRaw.events[4].value, 1);

        QCOMPARE(mockRaw.events[5].type, EV_SYN);
        QCOMPARE(mockRaw.events[5].code, SYN_REPORT);
        QCOMPARE(mockRaw.events[5].value, 0);

        QCOMPARE(mockRaw.events[6].type, EV_KEY);
        QCOMPARE(mockRaw.events[6].code, KEY_V);
        QCOMPARE(mockRaw.events[6].value, 0);

        QCOMPARE(mockRaw.events[7].type, EV_SYN);
        QCOMPARE(mockRaw.events[7].code, SYN_REPORT);
        QCOMPARE(mockRaw.events[7].value, 0);

        QCOMPARE(mockRaw.events[8].type, EV_KEY);
        QCOMPARE(mockRaw.events[8].code, KEY_LEFTSHIFT);
        QCOMPARE(mockRaw.events[8].value, 0);

        QCOMPARE(mockRaw.events[9].type, EV_SYN);
        QCOMPARE(mockRaw.events[9].code, SYN_REPORT);
        QCOMPARE(mockRaw.events[9].value, 0);

        QCOMPARE(mockRaw.events[10].type, EV_KEY);
        QCOMPARE(mockRaw.events[10].code, KEY_LEFTCTRL);
        QCOMPARE(mockRaw.events[10].value, 0);

        QCOMPARE(mockRaw.events[11].type, EV_SYN);
        QCOMPARE(mockRaw.events[11].code, SYN_REPORT);
        QCOMPARE(mockRaw.events[11].value, 0);
    }

    void testKeyboardMacroInjectorFailure() {
        MockRawDevice mockRaw;
        mockRaw.shouldFail = true;
        keyinjectord::KeyboardMacroInjector injector(mockRaw, 0ms);

        QVERIFY(!injector.sendCtrlShiftV());
    }

    void testSocketCredentialsVerification() {
        int sv[2];
        QCOMPARE(::socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sv), 0);

        struct ucred creds {};
        keyinjectord::AuthResult res = keyinjectord::AuthResult::InvalidFd;
        bool ok = keyinjectord::SocketCredentials::verifySocketAndGetPeer(sv[0], creds, &res);
        QVERIFY(ok);
        QCOMPARE(res, keyinjectord::AuthResult::Success);
        QCOMPARE(creds.uid, ::getuid());

        res = keyinjectord::AuthResult::InvalidFd;
        QVERIFY(keyinjectord::SocketCredentials::validatePeerIdentity(creds, &res));
        QCOMPARE(res, keyinjectord::AuthResult::Success);

        res = keyinjectord::AuthResult::Success;
        QVERIFY(!keyinjectord::SocketCredentials::verifySocketAndGetPeer(-1, creds, &res));
        QCOMPARE(res, keyinjectord::AuthResult::InvalidFd);

        ::close(sv[0]);
        ::close(sv[1]);
    }

    void testProcfsUtilsSelfExe() {
        keyinjectord::AuthResult res = keyinjectord::AuthResult::InvalidFd;
        std::filesystem::path selfPath;
        QVERIFY(keyinjectord::ProcfsUtils::readSelfExePath(selfPath, &res));
        QCOMPARE(res, keyinjectord::AuthResult::Success);
        QVERIFY(!selfPath.empty());

        struct stat st {};
        int fd = keyinjectord::ProcfsUtils::openSelfExeFd(&st, &res);
        QVERIFY(fd >= 0);
        QCOMPARE(res, keyinjectord::AuthResult::Success);
        QVERIFY(S_ISREG(st.st_mode));
        ::close(fd);
    }

    void testFsSecurityCheckerUntrustedPath() {
        QVERIFY(keyinjectord::FsSecurityChecker::isUntrustedPath("/tmp"));
        QVERIFY(keyinjectord::FsSecurityChecker::isUntrustedPath("/tmp/foo"));
        QVERIFY(keyinjectord::FsSecurityChecker::isUntrustedPath("/var/tmp"));
        QVERIFY(keyinjectord::FsSecurityChecker::isUntrustedPath("/var/tmp/nested/binary"));
        QVERIFY(keyinjectord::FsSecurityChecker::isUntrustedPath("/dev/shm"));
        QVERIFY(keyinjectord::FsSecurityChecker::isUntrustedPath("/dev/shm/test"));
        QVERIFY(keyinjectord::FsSecurityChecker::isUntrustedPath("/run/user/1000/qtscribe"));

        QVERIFY(!keyinjectord::FsSecurityChecker::isUntrustedPath("/usr/bin/qtscribe"));
        QVERIFY(!keyinjectord::FsSecurityChecker::isUntrustedPath("/opt/qtscribe"));
    }
};

QTEST_GUILESS_MAIN(TestIpcServer)
#include "test_ipc_server.moc"
