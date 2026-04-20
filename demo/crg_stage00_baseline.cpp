// ======================================================
// STAGE 0 — BASELINE
// ======================================================
//
// @intent:
// Establish an explicit baseline with no abstraction layer.
//
// @what_changed:
// Direct object calls only. No CRG mechanisms exist.
//
// @key_insight:
// There is no shared structure, no identity abstraction, and no composition system. 
// Everything is siloed and rigid.
//
// @what_is_not:
// No identity system, no behavior extraction, no runtime type abstraction, 
// no traversal or linking.
//
// @transition:
// Introduce emergent structure through object lifetime.
//
// @spoken_line:
// “At this stage, everything is explicit. Nothing is composed. It’s the rigid 
// world we are trying to escape.”
// ======================================================

#include <iostream>
#include <string>
#include <memory>

struct IHardware { virtual ~IHardware() = default; };

struct Drone : IHardware { 
    std::string id{"Drone"}; 
    void Diag() const { std::cout << id << "\n"; } 
};

struct HeavyLifter : IHardware { 
    std::string id{"HeavyLifter"}; 
    void Diag() const { std::cout << id << "\n"; } 
};

class FacilityManager {
public:
    void Execute(IHardware* h) {
        if (auto* d = dynamic_cast<Drone*>(h)) {
            std::cout << "[MANAGER] Testing rotors: " << d->id << "\n";
        } 
        else if (auto* l = dynamic_cast<HeavyLifter*>(h)) {
            std::cout << "[MANAGER] Testing hydraulics: " << l->id << "\n";
        }
    }
};

int main()
{
    FacilityManager manager;
    auto d = std::make_unique<Drone>();
    manager.Execute(d.get());
    return 0;
}
