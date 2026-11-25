#pragma once
#include <vector>
#include <string>

struct PersonalityTrait
{
    std::string id;
    int strength;
    std::string name;
    std::string description;
};

class PersonalityProfile
{
    private:
    std::vector<PersonalityTrait> traits;

    PersonalityTrait* findTraitMutable(const std::string& id);

    public:
    const std::vector<PersonalityTrait>& getTraits() const;
    
    const PersonalityTrait* findTrait(const std::string& id) const;

    bool setTraitStrength(const std::string& id, int strength);

    bool addTrait(const PersonalityTrait& trait);
};