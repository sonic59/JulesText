/*
  ==============================================================================

   This file is part of the JUCE library - "Jules' Utility Class Extensions"
   Copyright 2004-11 by Raw Material Software Ltd.

  ------------------------------------------------------------------------------

   JUCE can be redistributed and/or modified under the terms of the GNU General
   Public License (Version 2), as published by the Free Software Foundation.
   A copy of the license is included in the JUCE distribution, or can be found
   online at www.gnu.org/licenses.

   JUCE is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

  ------------------------------------------------------------------------------

   To release a closed-source product which uses JUCE, commercial licenses are
   available: visit www.rawmaterialsoftware.com/juce for more information.

  ==============================================================================
*/

#if ! JUCE_QUICKTIME
 #error "On the Mac, cameras use Quicktime, so if you turn on JUCE_USE_CAMERA, you also need to enable JUCE_QUICKTIME"
#endif

//==============================================================================
#define QTCaptureCallbackDelegate MakeObjCClassName(QTCaptureCallbackDelegate)

class QTCameraDeviceInteral;

END_JUCE_NAMESPACE

@interface QTCaptureCallbackDelegate    : NSObject
{
@public
    CameraDevice* owner;
    QTCameraDeviceInteral* internal;
    int64 firstPresentationTime;
    int64 averageTimeOffset;
}

- (QTCaptureCallbackDelegate*) initWithOwner: (CameraDevice*) owner internalDev: (QTCameraDeviceInteral*) d;
- (void) dealloc;

- (void) captureOutput: (QTCaptureOutput*) captureOutput
         didOutputVideoFrame: (CVImageBufferRef) videoFrame
         withSampleBuffer: (QTSampleBuffer*) sampleBuffer
         fromConnection: (QTCaptureConnection*) connection;

- (void) captureOutput: (QTCaptureFileOutput*) captureOutput
         didOutputSampleBuffer: (QTSampleBuffer*) sampleBuffer
         fromConnection: (QTCaptureConnection*) connection;
@end

BEGIN_JUCE_NAMESPACE

extern Image juce_createImageFromCIImage (CIImage* im, int w, int h);

//==============================================================================
class QTCameraDeviceInteral
{
public:
    QTCameraDeviceInteral (CameraDevice* owner, const int index)
        : input (nil),
          audioDevice (nil),
          audioInput (nil),
          session (nil),
          fileOutput (nil),
          imageOutput (nil)
    {
        JUCE_AUTORELEASEPOOL

        session = [[QTCaptureSession alloc] init];

        NSArray* devs = [QTCaptureDevice inputDevicesWithMediaType: QTMediaTypeVideo];
        device = (QTCaptureDevice*) [devs objectAtIndex: index];
        callbackDelegate = [[QTCaptureCallbackDelegate alloc] initWithOwner: owner
                                                                internalDev: this];

        NSError* err = nil;
        [device retain];
        [device open: &err];

        if (err == nil)
        {
            input      = [[QTCaptureDeviceInput alloc] initWithDevice: device];
            audioInput = [[QTCaptureDeviceInput alloc] initWithDevice: device];

            [session addInput: input error: &err];

            if (err == nil)
            {
                resetFile();

                imageOutput = [[QTCaptureDecompressedVideoOutput alloc] init];
                [imageOutput setDelegate: callbackDelegate];

                if (err == nil)
                {
                    [session startRunning];
                    return;
                }
            }
        }

        openingError = nsStringToJuce ([err description]);
        DBG (openingError);
    }

    ~QTCameraDeviceInteral()
    {
        [session stopRunning];
        [session removeOutput: imageOutput];

        [session release];
        [input release];
        [device release];
        [audioDevice release];
        [audioInput release];
        [fileOutput release];
        [imageOutput release];
        [callbackDelegate release];
    }

    void resetFile()
    {
        [fileOutput recordToOutputFileURL: nil];
        [session removeOutput: fileOutput];
        [fileOutput release];
        fileOutput = [[QTCaptureMovieFileOutput alloc] init];

        [session removeInput: audioInput];
        [audioInput release];
        audioInput = nil;
        [audioDevice release];
        audioDevice = nil;

        [fileOutput setDelegate: callbackDelegate];
    }

    void addDefaultAudioInput()
    {
        NSError* err = nil;
        audioDevice = [QTCaptureDevice defaultInputDeviceWithMediaType: QTMediaTypeSound];

        if ([audioDevice open: &err])
            [audioDevice retain];
        else
            audioDevice = nil;

        if (audioDevice != nil)
        {
            audioInput = [[QTCaptureDeviceInput alloc] initWithDevice: audioDevice];
            [session addInput: audioInput error: &err];
        }
    }

    void addListener (CameraDevice::Listener* listenerToAdd)
    {
        const ScopedLock sl (listenerLock);

        if (listeners.size() == 0)
            [session addOutput: imageOutput error: nil];

        listeners.addIfNotAlreadyThere (listenerToAdd);
    }

    void removeListener (CameraDevice::Listener* listenerToRemove)
    {
        const ScopedLock sl (listenerLock);
        listeners.removeValue (listenerToRemove);

        if (listeners.size() == 0)
            [session removeOutput: imageOutput];
    }

    void callListeners (CIImage* frame, int w, int h)
    {
        Image image (juce_createImageFromCIImage (frame, w, h));

        const ScopedLock sl (listenerLock);

        for (int i = listeners.size(); --i >= 0;)
        {
            CameraDevice::Listener* const l = listeners[i];

            if (l != nullptr)
                l->imageReceived (image);
        }
    }

    QTCaptureDevice* device;
    QTCaptureDeviceInput* input;
    QTCaptureDevice* audioDevice;
    QTCaptureDeviceInput* audioInput;
    QTCaptureSession* session;
    QTCaptureMovieFileOutput* fileOutput;
    QTCaptureDecompressedVideoOutput* imageOutput;
    QTCaptureCallbackDelegate* callbackDelegate;
    String openingError;

    Array<CameraDevice::Listener*> listeners;
    CriticalSection listenerLock;
};

END_JUCE_NAMESPACE
@implementation QTCaptureCallbackDelegate

- (QTCaptureCallbackDelegate*) initWithOwner: (CameraDevice*) owner_
                                 internalDev: (QTCameraDeviceInteral*) d
{
    [super init];
    owner = owner_;
    internal = d;
    firstPresentationTime = 0;
    averageTimeOffset = 0;
    return self;
}

- (void) dealloc
{
    [super dealloc];
}

- (void) captureOutput: (QTCaptureOutput*) captureOutput
         didOutputVideoFrame: (CVImageBufferRef) videoFrame
         withSampleBuffer: (QTSampleBuffer*) sampleBuffer
         fromConnection: (QTCaptureConnection*) connection
{
    if (internal->listeners.size() > 0)
    {
        JUCE_AUTORELEASEPOOL

        internal->callListeners ([CIImage imageWithCVImageBuffer: videoFrame],
                                 CVPixelBufferGetWidth (videoFrame),
                                 CVPixelBufferGetHeight (videoFrame));
    }
}

- (void) captureOutput: (QTCaptureFileOutput*) captureOutput
         didOutputSampleBuffer: (QTSampleBuffer*) sampleBuffer
         fromConnection: (QTCaptureConnection*) connection
{
    const Time now (Time::getCurrentTime());

   #if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5
    NSNumber* hosttime = (NSNumber*) [sampleBuffer attributeForKey: QTSampleBufferHostTimeAttribute];
   #else
    NSNumber* hosttime = (NSNumber*) [sampleBuffer attributeForKey: nsStringLiteral ("hostTime")];
   #endif

    int64 presentationTime = (hosttime != nil)
            ? ((int64) AudioConvertHostTimeToNanos ([hosttime unsignedLongLongValue]) / 1000000 + 40)
            : (([sampleBuffer presentationTime].timeValue * 1000) / [sampleBuffer presentationTime].timeScale + 50);

    const int64 timeDiff = now.toMilliseconds() - presentationTime;

    if (firstPresentationTime == 0)
    {
        firstPresentationTime = presentationTime;
        averageTimeOffset = timeDiff;
    }
    else
    {
        averageTimeOffset = (averageTimeOffset * 120 + timeDiff * 8) / 128;
    }
}

@end
BEGIN_JUCE_NAMESPACE

//==============================================================================
class QTCaptureViewerComp : public NSViewComponent
{
public:
    QTCaptureViewerComp (CameraDevice* const cameraDevice, QTCameraDeviceInteral* const internal)
    {
        JUCE_AUTORELEASEPOOL
        captureView = [[QTCaptureView alloc] init];
        [captureView setCaptureSession: internal->session];

        setSize (640, 480); //  xxx need to somehow get the movie size - how?
        setView (captureView);
    }

    ~QTCaptureViewerComp()
    {
        setView (0);
        [captureView setCaptureSession: nil];
        [captureView release];
    }

    QTCaptureView* captureView;
};

//==============================================================================
CameraDevice::CameraDevice (const String& name_, int index)
    : name (name_)
{
    isRecording = false;
    internal = new QTCameraDeviceInteral (this, index);
}

CameraDevice::~CameraDevice()
{
    stopRecording();
    delete static_cast <QTCameraDeviceInteral*> (internal);
    internal = nullptr;
}

Component* CameraDevice::createViewerComponent()
{
    return new QTCaptureViewerComp (this, static_cast <QTCameraDeviceInteral*> (internal));
}

String CameraDevice::getFileExtension()
{
    return ".mov";
}

void CameraDevice::startRecordingToFile (const File& file, int quality)
{
    stopRecording();

    QTCameraDeviceInteral* const d = static_cast <QTCameraDeviceInteral*> (internal);
    d->callbackDelegate->firstPresentationTime = 0;
    file.deleteFile();

    // In some versions of QT (e.g. on 10.5), if you record video without audio, the speed comes
    // out wrong, so we'll put some audio in there too..,
    d->addDefaultAudioInput();

    [d->session addOutput: d->fileOutput error: nil];

    NSEnumerator* connectionEnumerator = [[d->fileOutput connections] objectEnumerator];

    for (;;)
    {
        QTCaptureConnection* connection = [connectionEnumerator nextObject];
        if (connection == nil)
            break;

        QTCompressionOptions* options = nil;
        NSString* mediaType = [connection mediaType];

        if ([mediaType isEqualToString: QTMediaTypeVideo])
            options = [QTCompressionOptions compressionOptionsWithIdentifier:
                            quality >= 1 ? nsStringLiteral ("QTCompressionOptionsSD480SizeH264Video")
                                         : nsStringLiteral ("QTCompressionOptions240SizeH264Video")];
        else if ([mediaType isEqualToString: QTMediaTypeSound])
            options = [QTCompressionOptions compressionOptionsWithIdentifier: nsStringLiteral ("QTCompressionOptionsHighQualityAACAudio")];

        [d->fileOutput setCompressionOptions: options forConnection: connection];
    }

    [d->fileOutput recordToOutputFileURL: [NSURL fileURLWithPath: juceStringToNS (file.getFullPathName())]];
    isRecording = true;
}

Time CameraDevice::getTimeOfFirstRecordedFrame() const
{
    QTCameraDeviceInteral* const d = static_cast <QTCameraDeviceInteral*> (internal);
    if (d->callbackDelegate->firstPresentationTime != 0)
        return Time (d->callbackDelegate->firstPresentationTime + d->callbackDelegate->averageTimeOffset);

    return Time();
}

void CameraDevice::stopRecording()
{
    if (isRecording)
    {
        static_cast <QTCameraDeviceInteral*> (internal)->resetFile();
        isRecording = false;
    }
}

void CameraDevice::addListener (Listener* listenerToAdd)
{
    if (listenerToAdd != nullptr)
        static_cast <QTCameraDeviceInteral*> (internal)->addListener (listenerToAdd);
}

void CameraDevice::removeListener (Listener* listenerToRemove)
{
    if (listenerToRemove != nullptr)
        static_cast <QTCameraDeviceInteral*> (internal)->removeListener (listenerToRemove);
}

//==============================================================================
StringArray CameraDevice::getAvailableDevices()
{
    JUCE_AUTORELEASEPOOL

    StringArray results;
    NSArray* devs = [QTCaptureDevice inputDevicesWithMediaType: QTMediaTypeVideo];

    for (int i = 0; i < (int) [devs count]; ++i)
    {
        QTCaptureDevice* dev = (QTCaptureDevice*) [devs objectAtIndex: i];
        results.add (nsStringToJuce ([dev localizedDisplayName]));
    }

    return results;
}

CameraDevice* CameraDevice::openDevice (int index,
                                        int minWidth, int minHeight,
                                        int maxWidth, int maxHeight)
{
    ScopedPointer <CameraDevice> d (new CameraDevice (getAvailableDevices() [index], index));

    if (static_cast <QTCameraDeviceInteral*> (d->internal)->openingError.isEmpty())
        return d.release();

    return nullptr;
}
