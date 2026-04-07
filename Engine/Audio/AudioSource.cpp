#include "AudioSource.hpp"
#include "../Core/Logger.hpp"

#ifdef GE_HAS_OPENAL
#include <AL/al.h>
#include <AL/alc.h>
#endif

namespace GameEngine {

    AudioSource::AudioSource()
        : m_SourceID(0), m_Volume(1.0f), m_Pitch(1.0f), 
          m_Loop(false), m_Spatial(true), m_Position(0.0f) {
#ifdef GE_HAS_OPENAL
        alGenSources(1, &m_SourceID);
        
        if (m_SourceID) {
            alSourcef(m_SourceID, AL_GAIN, m_Volume);
            alSourcef(m_SourceID, AL_PITCH, m_Pitch);
            alSourcei(m_SourceID, AL_LOOPING, m_Loop ? AL_TRUE : AL_FALSE);
            alSource3f(m_SourceID, AL_POSITION, 0.0f, 0.0f, 0.0f);
        }
#endif
    }

    AudioSource::~AudioSource() {
#ifdef GE_HAS_OPENAL
        if (m_SourceID) {
            alDeleteSources(1, &m_SourceID);
        }
#endif
    }

    void AudioSource::SetClip(const Ref<AudioClip>& clip) {
        m_Clip = clip;
#ifdef GE_HAS_OPENAL
        if (m_SourceID && clip) {
            alSourcei(m_SourceID, AL_BUFFER, clip->GetBufferID());
        }
#endif
    }

    void AudioSource::Play() {
#ifdef GE_HAS_OPENAL
        if (m_SourceID) {
            alSourcePlay(m_SourceID);
        }
#endif
    }

    void AudioSource::Pause() {
#ifdef GE_HAS_OPENAL
        if (m_SourceID) {
            alSourcePause(m_SourceID);
        }
#endif
    }

    void AudioSource::Stop() {
#ifdef GE_HAS_OPENAL
        if (m_SourceID) {
            alSourceStop(m_SourceID);
        }
#endif
    }

    bool AudioSource::IsPlaying() const {
#ifdef GE_HAS_OPENAL
        if (m_SourceID) {
            ALint state;
            alGetSourcei(m_SourceID, AL_SOURCE_STATE, &state);
            return state == AL_PLAYING;
        }
#endif
        return false;
    }

    void AudioSource::SetVolume(float volume) {
        m_Volume = volume;
#ifdef GE_HAS_OPENAL
        if (m_SourceID) {
            alSourcef(m_SourceID, AL_GAIN, volume);
        }
#endif
    }

    void AudioSource::SetPitch(float pitch) {
        m_Pitch = pitch;
#ifdef GE_HAS_OPENAL
        if (m_SourceID) {
            alSourcef(m_SourceID, AL_PITCH, pitch);
        }
#endif
    }

    void AudioSource::SetLoop(bool loop) {
        m_Loop = loop;
#ifdef GE_HAS_OPENAL
        if (m_SourceID) {
            alSourcei(m_SourceID, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
        }
#endif
    }

    void AudioSource::SetPosition(const glm::vec3& position) {
        m_Position = position;
#ifdef GE_HAS_OPENAL
        if (m_SourceID && m_Spatial) {
            alSource3f(m_SourceID, AL_POSITION, position.x, position.y, position.z);
        }
#endif
    }

    void AudioSource::SetVelocity(const glm::vec3& velocity) {
#ifdef GE_HAS_OPENAL
        if (m_SourceID && m_Spatial) {
            alSource3f(m_SourceID, AL_VELOCITY, velocity.x, velocity.y, velocity.z);
        }
#endif
    }

    void AudioSource::SetSpatial(bool spatial) {
        m_Spatial = spatial;
#ifdef GE_HAS_OPENAL
        if (m_SourceID) {
            alSourcei(m_SourceID, AL_SOURCE_RELATIVE, spatial ? AL_FALSE : AL_TRUE);
        }
#endif
    }

    void AudioSource::SetRolloffFactor(float rolloff) {
#ifdef GE_HAS_OPENAL
        if (m_SourceID) {
            alSourcef(m_SourceID, AL_ROLLOFF_FACTOR, rolloff);
        }
#endif
    }

    void AudioSource::SetReferenceDistance(float distance) {
#ifdef GE_HAS_OPENAL
        if (m_SourceID) {
            alSourcef(m_SourceID, AL_REFERENCE_DISTANCE, distance);
        }
#endif
    }

    void AudioSource::SetMaxDistance(float distance) {
#ifdef GE_HAS_OPENAL
        if (m_SourceID) {
            alSourcef(m_SourceID, AL_MAX_DISTANCE, distance);
        }
#endif
    }

}
