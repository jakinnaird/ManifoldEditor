/*
* ManifoldEditor
*
* Copyright (c) 2023 James Kinnaird
*/

#pragma once

#include <wx/string.h>
#include <memory>

// Forward declaration for miniaudio types
struct ma_device;
struct ma_decoder;

class AudioSystem
{
public:
    struct StreamData;
    std::unique_ptr<ma_decoder> m_decoder;

private:
    ma_device* m_device;
    std::unique_ptr<StreamData> m_streamData;
    bool m_initialized;

public:
    AudioSystem();
    ~AudioSystem();

    // Plays a sound from the given wxFileSystem location (can be in zip or on disk)
    void playSound(const wxString& location);

    // Stops the currently playing sound
    void stopSound();

    // Optional: call to update audio system if needed
    void update();

private:
    void initDevice();
    void shutdownDevice();
};
