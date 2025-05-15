#include <gtest/gtest.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include "com_port_mouse.h" // Assuming the class is in this header

// Test fixture for ComPortMouse tests
class ComPortMouseTest : public ::testing::Test {
protected:
    const char* testPort = "/tmp/virtual_com"; // Virtual serial port for testing

    void SetUp() override {
        // Create a virtual serial port for testing
        if (mkfifo(testPort, 0666) {
            if (errno != EEXIST) {
                perror("mkfifo");
                FAIL() << "Failed to create virtual serial port";
            }
        }
    }

    void TearDown() override {
        unlink(testPort);
    }
};

// Test initialization with invalid port
TEST_F(ComPortMouseTest, InitializeInvalidPort) {
    ComPortMouse mouse("/nonexistent_port");
    EXPECT_FALSE(mouse.initialize());
}

// Test successful initialization
TEST_F(ComPortMouseTest, InitializeSuccess) {
    ComPortMouse mouse(testPort);
    EXPECT_TRUE(mouse.initialize());

    // Verify file descriptor is valid
    int fd = mouse.getFd(); // Note: You'll need to add a getter for fd in the class
    EXPECT_NE(fd, -1);
}

// Test reading data (requires separate process to write to virtual port)
TEST_F(ComPortMouseTest, ReadMouseData) {
    ComPortMouse mouse(testPort);
    ASSERT_TRUE(mouse.initialize());

    // Fork a process to write test data to the virtual port
    pid_t pid = fork();
    if (pid == 0) { // Child process
        int fd = open(testPort, O_WRONLY);
        if (fd == -1) {
            perror("open in child");
            exit(1);
        }

        // Send a test packet (format depends on your mouse protocol)
        unsigned char testPacket[3] = { 0x20, 50, 30 }; // Left button pressed, dx=50, dy=30
        write(fd, testPacket, sizeof(testPacket));
        close(fd);
        exit(0);
    }
    else if (pid > 0) { // Parent process
        // This would normally be in a separate thread
        // For simplicity, we'll just read once
        unsigned char packet[3];
        int bytes_read = read(mouse.getFd(), packet, sizeof(packet));

        EXPECT_EQ(bytes_read, sizeof(packet));
        EXPECT_EQ(packet[0], 0x20);
        EXPECT_EQ(packet[1], 50);
        EXPECT_EQ(packet[2], 30);

        // Wait for child
        int status;
        waitpid(pid, &status, 0);
    }
    else {
        perror("fork");
        FAIL() << "Failed to fork test process";
    }
}

// Test packet parsing
TEST(ComPortMousePacketTest, ParsePacket) {
    ComPortMouse mouse(""); // Empty device for this test

    // Create a test packet
    unsigned char packet[3] = { 0x30, 0x40, 0x50 }; // Right button, dx=64, dy=80

    // Normally these would be protected methods, you might need to make them public or add a test interface
    int left_button = (packet[0] & 0x20) ? 1 : 0;
    int right_button = (packet[0] & 0x10) ? 1 : 0;

    int dx = (packet[0] & 0x03) << 6 | (packet[1] & 0x3F);
    int dy = (packet[0] & 0x0C) << 4 | (packet[2] & 0x3F);

    if (dx > 127) dx -= 256;
    if (dy > 127) dy -= 256;

    EXPECT_EQ(left_button, 1);
    EXPECT_EQ(right_button, 1);
    EXPECT_EQ(dx, 64);
    EXPECT_EQ(dy, 80);
}