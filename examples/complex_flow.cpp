#include <bitsery/bitsery.h>
#include <bitsery/adapter/buffer.h>
//include traits for types, that we'll be using
#include <bitsery/traits/string.h>
#include <bitsery/traits/array.h>
#include <bitsery/traits/vector.h>
//include extension that will allow to have backward/forward compatibility
#include <bitsery/ext/growable.h>

namespace MyTypes {

    //define data
    enum Color:uint8_t { Red, Green, Blue };

    struct Vec3 { float x, y, z; };

    struct Weapon {
        std::string name;
        int16_t damage;
    private:
        //define serialize function as private, and give access to bitsery
        friend bitsery::Access;
        template <typename S>
        void serialize (S& s) {
            //forward/backward compatibility for monsters
            s.ext(*this, bitsery::ext::Growable{}, [&s](Weapon& o1) {
                s.text1b(o1.name, 20);
                s.value2b(o1.damage);
            });
        }
    };

    struct Monster {
        Vec3 pos;
        int16_t mana;
        int16_t hp;
        std::string name;
        std::vector<uint8_t> inventory;
        Color color;
        std::vector<Weapon> weapons;
        Weapon equipped;
        std::vector<Vec3> path;
    };

    template <typename S>
    void serialize(S& s, Vec3& o) {
        s.value4b(o.x);
        s.value4b(o.y);
        s.value4b(o.z);
    }

    template <typename S>
    void serialize (S& s, Monster& o) {
        //forward/backward compatibility for monsters
        s.ext(o, bitsery::ext::Growable{}, [&s](Monster& o1) {
            s.value1b(o1.color);
            s.value2b(o1.mana);
            s.value2b(o1.hp);
            s.object(o1.equipped);
            s.object(o1.pos);
            s.container(o1.path, 1000);
            s.container(o1.weapons, 100);
            s.container1b(o1.inventory, 50);
            s.text1b(o1.name, 20);
        });
    }
}

using namespace bitsery;

//use fixed-size buffer
using Buffer = std::array<uint8_t, 10000>;
using OutputAdapter = OutputBufferAdapter<Buffer>;
using InputAdapter = InputBufferAdapter<Buffer>;

//create configuration that enables session support, to work with "growable" extension
struct SessionsEnabled:public DefaultConfig {
    static constexpr bool BufferSessionsEnabled = true;
};

int main() {
    //set some random data
    MyTypes::Monster data{};
    data.name = "lew";
    data.weapons.push_back(MyTypes::Weapon{"GoodWeapon", 100});

    //create buffer to store data to
    Buffer buffer{};
    //since we're using different configuration, we cannot use quickSerialization function.
    BasicSerializer<AdapterWriter<OutputAdapter, SessionsEnabled>> ser{OutputAdapter{buffer}};
    ser.object(data);
    auto& w = AdapterAccess::getWriter(ser);
    w.flush();
    auto writtenSize = w.writtenBytesCount();


    MyTypes::Monster res{};
    //deserialize
    BasicDeserializer<AdapterReader<InputAdapter, SessionsEnabled>> des{InputAdapter{buffer.begin(), writtenSize}};
    des.object(res);
    auto& r = AdapterAccess::getReader(des);
    assert(r.error() == ReaderError::NoError && r.isCompletedSuccessfully());
}