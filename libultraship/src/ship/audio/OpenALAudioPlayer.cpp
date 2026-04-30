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
	
	while (!mAvailableBuffers.empty()) mAvailableBuffers.pop();

    alcDestroyContext(mContext);
    alcCloseDevice(mDevice);

    mSource = 0;
    mContext = nullptr;
    mDevice = nullptr;
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

    for (int i = 0; i < NUM_BUFFERS; i++) {
        mAvailableBuffers.push(mBuffers[i]);
    }

    SPDLOG_INFO("OpenAL initialized: {} ch, {} Hz", this->GetNumOutputChannels(), this->GetSampleLength());
    return true;
}

int OpenALAudioPlayer::Buffered() {
    ALint queued = 0;
    ALint processed = 0;
    alGetSourcei(mSource, AL_BUFFERS_QUEUED, &queued);
    alGetSourcei(mSource, AL_BUFFERS_PROCESSED, &processed);

    int bufferedBuffers = queued - processed;
    return bufferedBuffers * this->GetSampleLength();
}

void OpenALAudioPlayer::DoPlay(const uint8_t* buf, size_t len) {
    // Check if we have at least one free buffer to use
    ALint processed = 0;
    alGetSourcei(mSource, AL_BUFFERS_PROCESSED, &processed);
    
    // Insert back to the available queue any processed buffer
    while (processed > 0) {
        ALuint freeBuf = 0;
        alSourceUnqueueBuffers(mSource, 1, &freeBuf);
        mAvailableBuffers.push(freeBuf);
        processed--;
    }
	
    if (mAvailableBuffers.empty()) {
        return;
    }
    
    // Get first available free buffer and use it to deploy new audio
    ALuint freeBuffer = mAvailableBuffers.front();
    mAvailableBuffers.pop();
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
