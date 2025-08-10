#include <cstdio>
#include <iostream>
#include <vector>
#include <memory> // for smart pointers

#include <intrin.h>

#include "debugger.h"
#include "../src/util.h"

constexpr int SUCCESS = 0;

enum TestCase {
    changeFlag,
    changeMemory,
    readMemory,
    useHardwareBP,
    changeRegister,
    Size
};

using namespace RoboDBG;
class CrackMeSolver : public Debugger {
private:
    int testCase;
    uintptr_t inputDataPtr     = 0x00001007; //0xafafafaf
    uintptr_t passwordPtr      = 0x0000100E; //0x42424242
    uintptr_t jeInstructionPtr = 0x00001012;
    uintptr_t ret = 0x0000101e;
public:
    bool success = false;

    CrackMeSolver( int testId ) {
        this->testCase = testId;
        this->verbose = false;
    }


    virtual void onStart( uintptr_t imageBase, uintptr_t entryPoint ) {
        uint32_t password;
        switch(testCase) {
            //Cracking the application by changing the ZF before JE
            case TestCase::changeFlag:
                setBreakpoint( ASLR(jeInstructionPtr) );
                break;
                 //Cracking the application by replacing the MOV [ebp-4], 0x12121212 with the password "0x42424242".
                 //Leading to comparing it with itself
            case TestCase::changeMemory:
                writeMemory<uint32_t>( ASLR(inputDataPtr) ,0x42424242);
                break;
                //Cracking the application by reading the password and writing it to input
            case TestCase::readMemory:
                password = readMemory<uint32_t>( ASLR(passwordPtr) );
                writeMemory<uint32_t>( ASLR(inputDataPtr) ,password );
                break;
                //Same as changeFlag, but doing it with hardware breakpoints
            case TestCase::useHardwareBP:
                setHardwareBreakpoint(ASLR(jeInstructionPtr), DRReg::DR1, AccessType::EXECUTE, BreakpointLength::BYTE);
                break;
                //changing the return code by changing eax register
            case TestCase::changeRegister:
                setHardwareBreakpoint(ASLR(ret), DRReg::DR1, AccessType::EXECUTE, BreakpointLength::BYTE);
                break;
        }
    }

    virtual BreakpointAction onBreakpoint(uintptr_t address, HANDLE hThread) {
        switch(testCase) {
            case TestCase::changeFlag:
                setFlag(hThread, Flags32::ZF, true);
                break;
        }
        return BREAK;
    }

    virtual BreakpointAction onHardwareBreakpoint( uintptr_t address, HANDLE hThread, DRReg reg ) {
        switch(testCase) {
            case TestCase::useHardwareBP:
                setFlag(hThread, Flags32::ZF, true);
                break;
            case TestCase::changeRegister:
                setRegister(hThread, Register32::EAX,0x0000000);
                break;
        }
        return RESTORE;
    }

    virtual void onEnd( DWORD exitCode, DWORD pid ) {
        if(exitCode == 0) success = true;
    }
};


// ANSI color codes
#define GREEN   "\033[32m"
#define RED     "\033[31m"
#define YELLOW  "\033[33m"
#define RESET   "\033[0m"

int main() {
    Util::EnableDebugPrivilege();

    int totalTests = static_cast<int>(TestCase::Size);
    int passed = 0, failed = 0;

    std::cout << YELLOW << "[*] Running " << totalTests << " test cases...\n" << RESET;

    for (int i = 0; i < totalTests; ++i) {
        TestCase tc = static_cast<TestCase>(i);
        std::cout << "[*] Running test case: " << i << std::endl;

        std::unique_ptr<CrackMeSolver> ws = std::make_unique<CrackMeSolver>(tc);
        if (ws->startProcess("TestMe.exe") != 0) {
            std::cout << RED << "[-] Failed to start test application!\n[-] Tests aborted!" << RESET << std::endl;
            return -1;
        }

        ws->loop();

        if (ws->success) {
            std::cout << GREEN << "[+] Test " << i << " successful" << RESET << "\n";
            ++passed;
        } else {
            std::cout << RED << "[-] Test " << i << " failed" << RESET << "\n";
            ++failed;
        }
    }

    std::cout << YELLOW << "\n=== Test Summary ===\n" << RESET;
    std::cout << GREEN << "Passed: " << passed << RESET << std::endl;
    std::cout << RED   << "Failed: " << failed << RESET << std::endl;
    std::cout << YELLOW << "Total:  " << totalTests << RESET << std::endl;

    return (failed == 0) ? 0 : 1;
}
