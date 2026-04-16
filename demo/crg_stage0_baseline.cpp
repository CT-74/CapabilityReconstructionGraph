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

struct NPC { std::string name{"NPC"}; void Print() const { std::cout << name << "\n"; } };
struct Boss { std::string name{"Boss"}; void Print() const { std::cout << name << "\n"; } };
struct Player { std::string name{"Player"}; void Print() const { std::cout << name << "\n"; } };

int main()
{
    NPC npc; Boss boss; Player player;
    npc.Print(); boss.Print(); player.Print();
}