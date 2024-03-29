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

#if JUCE_QUICKTIME

END_JUCE_NAMESPACE

//==============================================================================
#define NonInterceptingQTMovieView MakeObjCClassName(NonInterceptingQTMovieView)

@interface NonInterceptingQTMovieView   : QTMovieView
{
}

- (id) initWithFrame: (NSRect) frame;
- (BOOL) acceptsFirstMouse: (NSEvent*) theEvent;
- (NSView*) hitTest: (NSPoint) p;

@end

@implementation NonInterceptingQTMovieView

- (id) initWithFrame: (NSRect) frame
{
    self = [super initWithFrame: frame];
    [self setNextResponder: [self superview]];
    return self;
}

- (void) dealloc
{
    [super dealloc];
}

- (NSView*) hitTest: (NSPoint) point
{
    return [self isControllerVisible] ? [super hitTest: point] : nil;
}

- (BOOL) acceptsFirstMouse: (NSEvent*) theEvent
{
    return YES;
}

@end

BEGIN_JUCE_NAMESPACE

//==============================================================================
#define theMovie (static_cast <QTMovie*> (movie))

//==============================================================================
QuickTimeMovieComponent::QuickTimeMovieComponent()
    : movie (0)
{
    setOpaque (true);
    setVisible (true);

    QTMovieView* view = [[NonInterceptingQTMovieView alloc] initWithFrame: NSMakeRect (0, 0, 100.0f, 100.0f)];
    setView (view);
    [view setWantsLayer: YES]; // prevents the view failing to redraw correctly when paused.
    [view release];
}

QuickTimeMovieComponent::~QuickTimeMovieComponent()
{
    closeMovie();
    setView (nil);
}

bool QuickTimeMovieComponent::isQuickTimeAvailable() noexcept
{
    return true;
}

static QTMovie* openMovieFromStream (InputStream* movieStream, File& movieFile)
{
    // unfortunately, QTMovie objects can only be created on the main thread..
    jassert (MessageManager::getInstance()->isThisTheMessageThread());

    QTMovie* movie = nil;

    FileInputStream* const fin = dynamic_cast <FileInputStream*> (movieStream);

    if (fin != nullptr)
    {
        movieFile = fin->getFile();
        movie = [QTMovie movieWithFile: juceStringToNS (movieFile.getFullPathName())
                                 error: nil];
    }
    else
    {
        MemoryBlock temp;
        movieStream->readIntoMemoryBlock (temp);

        const char* const suffixesToTry[] = { ".mov", ".mp3", ".avi", ".m4a" };

        for (int i = 0; i < numElementsInArray (suffixesToTry); ++i)
        {
            movie = [QTMovie movieWithDataReference: [QTDataReference dataReferenceWithReferenceToData: [NSData dataWithBytes: temp.getData()
                                                                                                                       length: temp.getSize()]
                                                                                                  name: [NSString stringWithUTF8String: suffixesToTry[i]]
                                                                                              MIMEType: nsEmptyString()]
                                              error: nil];

            if (movie != 0)
                break;
        }
    }

    return movie;
}

bool QuickTimeMovieComponent::loadMovie (const File& movieFile_,
                                         const bool isControllerVisible_)
{
    return loadMovie ((InputStream*) movieFile_.createInputStream(), isControllerVisible_);
}

bool QuickTimeMovieComponent::loadMovie (InputStream* movieStream,
                                         const bool controllerVisible_)
{
    closeMovie();

    if (getPeer() == nullptr)
    {
        // To open a movie, this component must be visible inside a functioning window, so that
        // the QT control can be assigned to the window.
        jassertfalse;
        return false;
    }

    movie = openMovieFromStream (movieStream, movieFile);

    [theMovie retain];
    QTMovieView* view = (QTMovieView*) getView();
    [view setMovie: theMovie];
    [view setControllerVisible: controllerVisible_];
    setLooping (looping);

    return movie != nil;
}

bool QuickTimeMovieComponent::loadMovie (const URL& movieURL,
                                         const bool isControllerVisible_)
{
    // unfortunately, QTMovie objects can only be created on the main thread..
    jassert (MessageManager::getInstance()->isThisTheMessageThread());

    closeMovie();

    if (getPeer() == nullptr)
    {
        // To open a movie, this component must be visible inside a functioning window, so that
        // the QT control can be assigned to the window.
        jassertfalse;
        return false;
    }

    NSURL* url = [NSURL URLWithString: juceStringToNS (movieURL.toString (true))];
    NSError* err;
    if ([QTMovie canInitWithURL: url])
        movie = [QTMovie movieWithURL: url error: &err];

    [theMovie retain];
    QTMovieView* view = (QTMovieView*) getView();
    [view setMovie: theMovie];
    [view setControllerVisible: controllerVisible];
    setLooping (looping);

    return movie != nil;
}

void QuickTimeMovieComponent::closeMovie()
{
    stop();
    QTMovieView* view = (QTMovieView*) getView();
    [view setMovie: nil];
    [theMovie release];
    movie = 0;
    movieFile = File::nonexistent;
}

bool QuickTimeMovieComponent::isMovieOpen() const
{
    return movie != nil;
}

File QuickTimeMovieComponent::getCurrentMovieFile() const
{
    return movieFile;
}

void QuickTimeMovieComponent::play()
{
    [theMovie play];
}

void QuickTimeMovieComponent::stop()
{
    [theMovie stop];
}

bool QuickTimeMovieComponent::isPlaying() const
{
    return movie != 0  && [theMovie rate] != 0;
}

void QuickTimeMovieComponent::setPosition (const double seconds)
{
    if (movie != 0)
    {
        QTTime t;
        t.timeValue = (uint64) (100000.0 * seconds);
        t.timeScale = 100000;
        t.flags = 0;

        [theMovie setCurrentTime: t];
    }
}

double QuickTimeMovieComponent::getPosition() const
{
    if (movie == 0)
        return 0.0;

    QTTime t = [theMovie currentTime];
    return t.timeValue / (double) t.timeScale;
}

void QuickTimeMovieComponent::setSpeed (const float newSpeed)
{
    [theMovie setRate: newSpeed];
}

double QuickTimeMovieComponent::getMovieDuration() const
{
    if (movie == 0)
        return 0.0;

    QTTime t = [theMovie duration];
    return t.timeValue / (double) t.timeScale;
}

void QuickTimeMovieComponent::setLooping (const bool shouldLoop)
{
    looping = shouldLoop;

    [theMovie setAttribute: [NSNumber numberWithBool: shouldLoop]
                    forKey: QTMovieLoopsAttribute];
}

bool QuickTimeMovieComponent::isLooping() const
{
    return looping;
}

void QuickTimeMovieComponent::setMovieVolume (const float newVolume)
{
    [theMovie setVolume: newVolume];
}

float QuickTimeMovieComponent::getMovieVolume() const
{
    return movie != 0 ? [theMovie volume] : 0.0f;
}

void QuickTimeMovieComponent::getMovieNormalSize (int& width, int& height) const
{
    width = 0;
    height = 0;

    if (movie != 0)
    {
        NSSize s = [[theMovie attributeForKey: QTMovieNaturalSizeAttribute] sizeValue];
        width = (int) s.width;
        height = (int) s.height;
    }
}

void QuickTimeMovieComponent::paint (Graphics& g)
{
    if (movie == 0)
        g.fillAll (Colours::black);
}

bool QuickTimeMovieComponent::isControllerVisible() const
{
    return controllerVisible;
}

//==============================================================================
void QuickTimeMovieComponent::goToStart()
{
    setPosition (0.0);
}

void QuickTimeMovieComponent::setBoundsWithCorrectAspectRatio (const Rectangle<int>& spaceToFitWithin,
                                                               const RectanglePlacement& placement)
{
    int normalWidth, normalHeight;
    getMovieNormalSize (normalWidth, normalHeight);

    const Rectangle<int> normalSize (0, 0, normalWidth, normalHeight);

    if (! (spaceToFitWithin.isEmpty() || normalSize.isEmpty()))
        setBounds (placement.appliedTo (normalSize, spaceToFitWithin));
    else
        setBounds (spaceToFitWithin);
}


//==============================================================================
#if ! (JUCE_MAC && JUCE_64BIT)

bool juce_OpenQuickTimeMovieFromStream (InputStream* movieStream, Movie& result, Handle& dataHandle)
{
    if (movieStream == nullptr)
        return false;

    File file;
    QTMovie* movie = openMovieFromStream (movieStream, file);

    if (movie != nil)
        result = [movie quickTimeMovie];

    return movie != nil;
}

#endif
#endif
