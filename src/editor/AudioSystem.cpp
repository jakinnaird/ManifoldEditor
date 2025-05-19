/*
* ManifoldEditor
*
* Copyright (c) 2023 James Kinnaird
*/

#include "AudioSystem.hpp"

#define MA_NO_WASAPI // this doesn't seem to work correctly
#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#include <wx/filesys.h>
#include <wx/log.h>
#include <wx/mstream.h>

#include <vector>
#include <memory>
#include <cstring>

struct AudioSystem::StreamData
{
    std::unique_ptr<wxInputStream> stream;
    wxString location;
    std::vector<uint8_t> buffer; // For temporary read buffer
};

// miniaudio callbacks for custom stream
static ma_result wx_read_proc(ma_decoder* pDecoder, void* pBufferOut, size_t bytesToRead, size_t* pBytesRead)
{
    auto* data = static_cast<AudioSystem::StreamData*>(pDecoder->pUserData);
    if (!data || !data->stream)
        return MA_ERROR;
    size_t actuallyRead = data->stream->Read(pBufferOut, bytesToRead).LastRead();
    if (pBytesRead)
        *pBytesRead = actuallyRead;
    return actuallyRead > 0 ? MA_SUCCESS : MA_AT_END;
}

static ma_result wx_seek_proc(ma_decoder* pDecoder, ma_int64 byteOffset, ma_seek_origin origin)
{
    auto* data = static_cast<AudioSystem::StreamData*>(pDecoder->pUserData);
    if (!data || !data->stream)
        return MA_ERROR;
    wxSeekMode mode = (origin == ma_seek_origin_start) ? wxFromStart : wxFromCurrent;
    return (data->stream->SeekI(byteOffset, mode) != wxInvalidOffset) ? MA_SUCCESS : MA_ERROR;
}

static ma_result wx_tell_proc(ma_decoder* /*pDecoder*/, ma_int64* pCursor, void* pUserData)
{
    auto* data = static_cast<AudioSystem::StreamData*>(pUserData);
    if (!data || !data->stream || !pCursor)
        return MA_ERROR;
    *pCursor = data->stream->TellI();
    return MA_SUCCESS;
}

// Device data callback for miniaudio
static void dataCallback(ma_device* pDevice, void* pOutput, const void* /*pInput*/, ma_uint32 frameCount)
{
    auto* self = static_cast<AudioSystem*>(pDevice->pUserData);
    if (!self || !self->m_decoder)
    {
        std::memset(pOutput, 0, frameCount * sizeof(float) * 2);
        return;
    }
    ma_decoder_read_pcm_frames(self->m_decoder.get(), pOutput, frameCount, nullptr);
}

// AudioSystem Implementation
AudioSystem::AudioSystem()
{
    initDevice();
}

AudioSystem::~AudioSystem()
{
    shutdownDevice();
}

void AudioSystem::initDevice()
{
    if (m_initialized)
        return;

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.playback.channels = 2;
    config.sampleRate = 44100;
    config.dataCallback = dataCallback;
    config.pUserData = this;
    m_device = new ma_device;
    if (ma_device_init(nullptr, &config, m_device) == MA_SUCCESS)
    {
        m_initialized = true;
        ma_device_start(m_device);
    }
    else
    {
        delete m_device;
        m_device = nullptr;
    }
}

void AudioSystem::shutdownDevice()
{
    if (m_device)
    {
        ma_device_uninit(m_device);
        delete m_device;
        m_device = nullptr;
    }
    m_initialized = false;
}

void AudioSystem::playSound(const wxString& location)
{
    if (!m_initialized)
        return;

    // Open file via wxFileSystem (supports zip, etc)
    wxFileSystem fileSystem;
    std::unique_ptr<wxFSFile> fsFile(fileSystem.OpenFile(location));
    if (!fsFile)
    {
        wxLogWarning(_("Failed to open file: %s"), location.c_str());
        return;
    }

    std::unique_ptr<wxInputStream> stream(fsFile->DetachStream());
    if (!stream)
    {
        wxLogWarning(_("Failed to open file: %s"), location.c_str());
        return;
    }

    // Prepare stream data for miniaudio
    m_streamData.reset(new StreamData());
    m_streamData->stream = std::move(stream);
    m_streamData->location = location;

    // Setup decoder config
    ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_f32, 2, 44100);
    m_decoder.reset(new ma_decoder);
    if (ma_decoder_init(wx_read_proc, wx_seek_proc, m_streamData.get(), &decoderConfig, m_decoder.get()) != MA_SUCCESS)
    {
        m_decoder.reset();
        m_streamData.reset();
        return;
    }

    // No need to set device callback or user data here; already set in initDevice().
    // Just update m_decoder and the callback will use the new decoder.
}

void AudioSystem::stopSound()
{
    if (m_decoder)
    {
        ma_decoder_uninit(m_decoder.get());
        m_decoder.reset();
    }
}

void AudioSystem::update()
{
    // No-op for now. Could be used for polling or cleanup if needed.
}

void AudioSystem::getSoundMetadata(const wxString& location, uint32_t& sampleRate, uint32_t& channels)
{
    sampleRate = 0;
    channels = 0;

    // Open file via wxFileSystem (supports zip, etc)
    wxFileSystem fileSystem;
    std::unique_ptr<wxFSFile> fsFile(fileSystem.OpenFile(location));
    if (!fsFile)
    {
        wxLogWarning(_("Failed to open file: %s"), location.c_str());
        return;
    }

    std::unique_ptr<wxInputStream> stream(fsFile->DetachStream());
    if (!stream)
    {
        wxLogWarning(_("Failed to open file: %s"), location.c_str());
        return;
    }

    m_streamData.reset(new StreamData());
    m_streamData->stream = std::move(stream);
    m_streamData->location = location;

    // Get the sound metadata
    ma_decoder decoder;
    if (ma_decoder_init(wx_read_proc, wx_seek_proc, m_streamData.get(), nullptr, &decoder) != MA_SUCCESS)
    {
        wxLogWarning(_("Failed to initialize decoder for file: %s"), location.c_str());
        return;
    }

    sampleRate = decoder.outputSampleRate;
    channels = decoder.outputChannels;
    ma_decoder_uninit(&decoder);
    m_streamData.reset();
}
