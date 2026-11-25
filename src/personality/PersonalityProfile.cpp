#include "personality/PersonalityProfile.hpp"

PersonalityTrait* PersonalityProfile::findTraitMutable(const std::string& id)
{
    for(auto& trait : traits)
    {
        if(trait.id == id)
            return &trait;
    }
    return nullptr;
}

const PersonalityTrait* PersonalityProfile::findTrait(const std::string& id) const
{
    for(const auto& trait : traits)
    {
        if(trait.id == id)
            return &trait;
    }
    return nullptr;
}

bool PersonalityProfile::setTraitStrength(
    const std::string& id,
    int strength
)
{
    if(strength > 100 || strength < 0)
        return false;
    PersonalityTrait* trait = findTraitMutable(id);
    if(trait != nullptr)
    {
        trait->strength = strength;
        return true;   
    }
    return false;
}

bool PersonalityProfile::addTrait(const PersonalityTrait& trait)
{
    if (trait.id.empty() || trait.name.empty())
    {
        return false;
    }

    if (trait.strength < 0 || trait.strength > 100)
    {
        return false;
    }

    if (findTrait(trait.id) != nullptr)
    {
        return false;
    }

    traits.push_back(trait);

    return true;
}