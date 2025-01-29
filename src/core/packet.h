#ifndef _PACKET_H
#define _PACKET_H

#include <cstdint>
#include <cstring>
#include <vector>

#include "message.h"

namespace Msg {
    class Packet {
    public:
        struct Header {
            Opcodes opcode = Opcodes::MAX;
            uint16_t dataSize = 0;

            inline bool IsValid() const {
                return (opcode >= Opcodes::Echo && opcode < Opcodes::MAX);
            }
        };

    private:
        class BuildProxy {
        private:
            std::vector<uint8_t> buffer;
        public:
            BuildProxy(const Opcodes opcode) {
                buffer.resize(sizeof(Header));
                auto* header = reinterpret_cast<Header*>(buffer.data());
                header->opcode = opcode; 
            }

            inline void Append(const void* dataPtr, const uint16_t size) {
                const auto bufferSize = buffer.size();
                buffer.resize(bufferSize + size);
                std::memcpy(buffer.data() + bufferSize, dataPtr, size);
            }

            template<typename T>
            inline void Append(const T data) {
                buffer.resize(buffer.size() + sizeof(data));
                T* dataDest = reinterpret_cast<T*>(buffer.data() + buffer.size() - sizeof(data));

                *dataDest = data;
            }

            template<typename T>
            inline void Append(const T& data) {
                buffer.resize(buffer.size() + sizeof(data));
                T* dataDest = reinterpret_cast<T*>(buffer.data() + buffer.size() - sizeof(data));

                *dataDest = data;
            }

            inline const Packet* Complete() {
                Packet* packet = reinterpret_cast<Packet*>(buffer.data());
                packet->header.dataSize = buffer.size() - sizeof(Header);

                return packet;
            }
        };

        Header header;
        uint8_t data[];

    public:
        static BuildProxy Build(const Opcodes opcode) { return BuildProxy(opcode); }

        inline const Header& GetHeader() const { return header; }
        inline uint16_t GetSize() const  { return header.dataSize + sizeof(Header); }
        inline uint16_t GetDataSize() const { return header.dataSize; }

        template<typename T>
        inline T* GetDataAs() { return reinterpret_cast<T*>(&data[0]); }

        template<typename T>
        inline const T* GetDataAs() const { return reinterpret_cast<T*>(&data[0]); }

        inline bool Is(const Opcodes opcode) const { return header.opcode == opcode; }
    };
}

#endif
