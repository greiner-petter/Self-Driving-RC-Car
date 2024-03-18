#include "../common/ocConst.h"
#include "../common/ocMember.h"
#include "../common/ocPacket.h"

#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include <unistd.h>

static bool parse_command(std::string command, ocPacket *packet)
{
    std::istringstream buf(command);
    std::istream_iterator<std::string> start(buf), end;

    std::vector<std::string> tokens(start, end);

    if (1 <= tokens.size())
    {
        int input = std::stoi(tokens[0], nullptr, 16);
        if (0 <= input && input < ((1 << 16) - 1))
        {
            packet->set_message_id((ocMessageId) input);
        }
        else
        {
            return false;
        }
    }
    if (2 <= tokens.size())
    {
        int input = std::stoi(tokens[1], nullptr, 16);
        if (0 <= input)
        {
            packet->clear_and_edit().write(input);
        }
        else
        {
            return false;
        }
    }
    if (3 <= tokens.size()) return false;

    return true;
}

static void print_usage()
{
    std::cout << "Usage: command_line [Opcode [Message]]\n";
    std::cout << "Opcode and Message have to be hexadecimal numbers.\n";
    std::cout << "If no parameters are given, command_line will start in interactive mode.\n";
    std::cout << "In interactive mode you can repeatedly write messages in the same Opcode [Message] form.\n";
    std::cout << "To exit interactive mode, hit ctrl+c oder type q as the command.\n";
    std::cout << '\n';
    std::cout << "Example: command_line C9 1\n";
    std::cout << "  Sends a message saying that button 1 was pressed.\n";
    std::cout << "Example: command_line 41 8080\n";
    std::cout << "  Sends a message commanding to steer straight.\n";
}

int main (int argc, char** argv)
{
    ocMember member(ocMemberId::Command_Line, "Command Line");
    member.attach();

    ocLogger *logger = member.get_logger();
    ocPacket p;

    if (1 < argc)
    {
        std::string line;
        for  (int i = 1; i < argc; ++i)
        {
            if (1 != i) line += " ";
            line += argv[i];
        }
        if (parse_command(line, &p))
        {
            member.get_socket()->send_packet(p);
        }
        else
        {
            logger->error("Could not parse the input.");
            print_usage();
        }
    }
    else
    {
        print_usage();
        std::string line;
        while (true)
        {
            std::cout << "cli> ";
            if (!std::getline(std::cin, line)) break;
            if ("q" == line) break;

            if (parse_command(line, &p))
            {
                member.get_socket()->send_packet(p);
            }
            else
            {
                logger->error("Could not parse the input.");
            }
        }
    }

    member.get_socket()->send(ocMessageId::Disconnect_Me);
}
