#pragma once
#include "AudioPlayer.h"
#include <AL/al.h>
#include <AL/alc.h>
#include <atomic>
#include <queue>

namespace Ship {

class OpenALAudioPlayer final : public AudioPlayer {
  public:
    OpenALAudioPlayer(AudioSettings settings) : AudioPlayer(settings) {}
    ~OpenALAudioPlayer();
    int Buffered() override;

  protected:
    bool DoInit() override;
    void DoClose() override;
    void DoPlay(const uint8_t* buf, size_t len) override;

  private:
    ALCdevice *mDevice = nullptr;
    ALCcontext *mContext = nullptr;
    ALuint mSource  = 0;
	std::queue<ALuint> mAvailableBuffers;

    static constexpr int NUM_BUFFERS = 8;
    ALuint mBuffers[NUM_BUFFERS] = {};
};

} // namespace Ship
