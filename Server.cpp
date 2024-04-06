#include "thread.h"
#include "socketserver.h"
#include "socket.h"
#include "Game/chess.h"
#include <stdlib.h>
#include <time.h>
#include <list>
#include <vector>
#include <iostream>
#include <mutex>
#include <condition_variable>

using namespace Sync;

std::mutex mtx;                      // Mutex for thread synchronization
std::condition_variable cv;          // Condition variable for notifying
std::vector<Socket *> clientSockets; // Vector to store client sockets

class GameManager : public Thread
{
private:
    // Socket& sock;
    // bool& flag;
    Socket *player1, *player2; // do we want to use reference instead of pointers
    Board board;               // Chess board

public:
    GameManager(Socket *p1, Socket *p2) : player1(p1), player2(p2)
    {
        // Initialize the GameManager with two player sockets
    }
    ~GameManager() {} // destructor to kill thread

    // manage thread from game
    virtual long ThreadMain() override
    {
        std::cout << "Chess game started between two players." << std::endl;

        // Initialize the chess board
        board.setBoard();

        while (true)
        { // A loop to handle the game interaction
            if (!processPlayerMove(player1))
            {
                break; // Exit the loop if there's an issue or the game ends
            }

            if (!processPlayerMove(player2))
            {
                break; // Exit the loop if there's an issue or the game ends
            }

            // Additional conditions to exit based on the game state can be added here
        }

        std::cout << "Game ended." << std::endl;

        // Cleanup
        player1->Close();
        delete player1;
        player2->Close();
        delete player2;
        return 0;
    }

    bool processPlayerMove(Socket *player)
    {
        // 1. Read the move from the player's socket
        // 2. Update the board with the move
        // 3. Send the updated board state to both players
        return true;
    }
};

class ThreadSocket : public Thread
{
public:
    ThreadSocket(Socket *socket_connect_req) : socket(socket_connect_req) {}

    long ThreadMain() override
    {
        // Read a message from the client
        ByteArray received_buffered_msg;
        int number_bytes_received = socket->Read(received_buffered_msg);
        std::string msg_client = received_buffered_msg.ToString();

        // Check the message content
        if (msg_client != "server")
        {
            // // Write manipulated msg back to socket
            // ByteArray buffered_msg("Waiting for another player");
            // int success_writing = socket->Write(buffered_msg);
            // // printf("%d number bytes written to client\n", success_writing);
            // return success_writing;

            // Notify the client that we're waiting for another player
            ByteArray buffered_msg("Waiting for another player");
            socket->Write(buffered_msg);
        }
        else
        {
            // printf("user entered done, terminate");
            Bye();
            // printf("returning here");
            // return 0;
        }
        // Return the number of bytes received or processed
        return number_bytes_received;
    }
    void Bye()
    {
        // terminationEvent.Wait();
        // socket->Close(); // closing the connection
        // delete socket;
        // Close the socket and release resources
        if (socket)
        {
            socket->Close();
            delete socket;
            socket = nullptr; // Prevent double deletion and undefined behavior
        }
    }
    ~ThreadSocket()
    {
        // waiting on Sync::Event termination event to make a block
        Bye();
    }

private:
    Socket *socket;
    // If manipulateString is defined outside, you may need to declare it here or include its header
};

int main(void)
{
    SocketServer sockServer(3000); // create server socket
    std::cout << "I am server" << std::endl;

    try
    {
        while (true)
        {
            Socket *newClientSocket = new Socket(sockServer.Accept());
            std::cout << "Client connected!" << std::endl;

            // Protecting the access to clientSockets vector with a mutex
            {
                std::lock_guard<std::mutex> lock(mtx); // Protect the clientSockets vector
                clientSockets.push_back(newClientSocket);
            }

            // Check if we have two clients to start a game
            if (clientSockets.size() >= 2)
            {
                // Create a game session with the first two clients
                GameManager *game = new GameManager(clientSockets[0], clientSockets[1]);
                game->Start(); // Start game session in a new thread

                // Remove the clients from the waiting list
                {
                    std::lock_guard<std::mutex> lock(mtx);
                    clientSockets.erase(clientSockets.begin(), clientSockets.begin() + 2);
                }
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Standard exception caught: " << e.what() << std::endl;
        // Clean up any remaining sockets
        for (auto *socket : clientSockets)
        {
            socket->Close();
            delete socket;
        }
        sockServer.Shutdown(); // Shutdown the server on exception
    }
    return 0;
}