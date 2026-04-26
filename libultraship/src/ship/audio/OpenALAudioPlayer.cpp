#include "ship/audio/OpenALAudioPlayer.h"
#include <spdlog/spdlog.h>

namespace Ship {

OpenALAudioPlayer::~OpenALAudioPlayer() {
    SPDLOG_TRACE("destruct OpenAL audio player");
    DoClose();
}

void OpenALAudioPlayer::DoClose() {
    if (!mDevice) {
        return;
    }

    alSourceStop(mSource);
    alSourcei(mSource, AL_BUFFER, 0);
    alDeleteSources(1, &mSource);
    alDeleteBuffers(NUM_BUFFERS, mBuffers);

    alcDestroyContext(mContext);
    alcCloseDevice(mDevice);

    mSource      = 0;
    mContext     = nullptr;
    mDevice      = nullptr;
}

bool OpenALAudioPlayer::DoInit() {
    mDevice = alcOpenDevice(nullptr);
    if (!mDevice) {
        SPDLOG_ERROR("alcOpenDevice failed");
        return false;
    }

    mContext = alcCreateContext(mDevice, nullptr);
    if (!mContext) {
        SPDLOG_ERROR("alcCreateContext failed");
        alcCloseDevice(mDevice);
        mDevice = nullptr;
        return false;
    }
    alcMakeContextCurrent(mContext);

    alGenBuffers(NUM_BUFFERS, mBuffers);
    alGenSources(1, &mSource);
    ALenum fmt = (this->GetNumOutputChannels() == 2) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;

    // Queue buffers used as a circular pool with full silent data
    std::vector<int16_t> silence(this->GetSampleLength() * this->GetNumOutputChannels(), 0);

    for (int i = 0; i < NUM_BUFFERS; i++) {
        alBufferData(mBuffers[i], fmt, silence.data(), static_cast<ALsizei>(silence.size() * sizeof(int16_t)), this->GetSampleRate());
    }
    alSourceQueueBuffers(mSource, NUM_BUFFERS, mBuffers);
    alSourcePlay(mSource);

    SPDLOG_INFO("OpenAL initialized: {} ch, {} Hz", ch, gameRate);
    return true;
}

int OpenALAudioPlayer::Buffered() {
    ALint queued    = 0;
    ALint processed = 0;
    alGetSourcei(mSource, AL_BUFFERS_QUEUED,    &queued);
    alGetSourcei(mSource, AL_BUFFERS_PROCESSED, &processed);

    int bufferedBuffers = queued - processed;
    return bufferedBuffers * this->GetSampleLength();
}

void OpenALAudioPlayer::DoPlay(const uint8_t* buf, size_t len) {
    // Check if we have at least one free buffer to use
    ALint processed = 0;
    alGetSourcei(mSource, AL_BUFFERS_PROCESSED, &processed);
    if (processed == 0) {
        return;
    }
    
    // Get first available free buffer and use it to deploy new audio
    ALuint freeBuffer = 0;
    alSourceUnqueueBuffers(mSource, 1, &freeBuffer);
    ALenum fmt = (this->GetNumOutputChannels() == 2) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
    alBufferData(freeBuffer, fmt, buf, static_cast<ALsizei>(len), this->GetSampleRate());
    alSourceQueueBuffers(mSource, 1, &freeBuffer);

    // Restart the source if it finished all previous buffers
    ALint state = 0;
    alGetSourcei(mSource, AL_SOURCE_STATE, &state);
    if (state != AL_PLAYING) {
        alSourcePlay(mSource);
    }
}

} // namespace Ship