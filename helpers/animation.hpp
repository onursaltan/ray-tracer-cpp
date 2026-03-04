#ifndef ANIMATION_HPP
#define ANIMATION_HPP

#include <vector>
#include <cmath>
#include "math.hpp"

struct Keyframe
{
    float time;
    Vec3 position;
    Vec3 rotation;
    Vec3 scale;
};

class Animation
{
private:
    std::vector<Keyframe> keyframes;
    bool looping = true;

public:
    void addKeyframe(const Keyframe &kf)
    {
        keyframes.push_back(kf);
    }

    Keyframe interpolate(float time)
    {
        if (keyframes.empty())
            return Keyframe{0, Vec3(0, 0, 0), Vec3(0, 0, 0), Vec3(1, 1, 1)};

        if (looping && keyframes.back().time > 0)
        {
            time = fmod(time, keyframes.back().time);
        }

        for (size_t i = 0; i < keyframes.size() - 1; i++)
        {
            if (time >= keyframes[i].time && time <= keyframes[i + 1].time)
            {
                const Keyframe &kf1 = keyframes[i];
                const Keyframe &kf2 = keyframes[i + 1];

                float duration = kf2.time - kf1.time;
                float t = (time - kf1.time) / duration;

                Keyframe result;
                result.time = time;
                result.position = kf1.position + (kf2.position - kf1.position) * t;
                result.rotation = kf1.rotation + (kf2.rotation - kf1.rotation) * t;
                result.scale = kf1.scale + (kf2.scale - kf1.scale) * t;

                return result;
            }
        }

        return keyframes.back();
    }

    void setLooping(bool loop) { looping = loop; }
    float getDuration() const { return keyframes.empty() ? 0 : keyframes.back().time; }
};

#endif
