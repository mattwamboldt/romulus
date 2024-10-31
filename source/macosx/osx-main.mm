#include <stdio.h>
#include <AppKit/AppKit.h>

static float GlobalRenderWidth = 1024;
static float GlobalRenderHeight = 768;

static bool isRunning = true;

// TODO: look at AppDelegate and WindowDelegate to see what handlers make sense
@interface MainWindowDelegate: NSObject<NSWindowDelegate>
@end

@implementation MainWindowDelegate

- (void)windowWillClose:(NSNotification *)notification
{
    isRunning = false;
}
@end

// TODO: Add platform services functions, mainly file I/O

int main(int argc, const char* argv[])
{
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    MainWindowDelegate* mainWindowDelegate = [[MainWindowDelegate alloc] init];
    NSRect screenRect = [[NSScreen mainScreen] frame];
    NSRect initialFrame = NSMakeRect(
        (screenRect.size.width - GlobalRenderWidth) * 0.5,
        (screenRect.size.height - GlobalRenderHeight) * 0.5,
        GlobalRenderWidth, GlobalRenderHeight);
    NSWindow* window = [[NSWindow alloc] initWithContentRect:initialFrame
                                         styleMask: NSWindowStyleMaskTitled |
                                                    NSWindowStyleMaskClosable |
                                                    NSWindowStyleMaskResizable |
                                                    NSWindowStyleMaskMiniaturizable
                                         backing:NSBackingStoreBuffered
                                         defer:NO];

    [window setBackgroundColor:NSColor.redColor];
    [window setTitle: @"Romulus"];
    [window setDelegate: mainWindowDelegate];
    [window makeKeyAndOrderFront:NSApp];

    // TODO: Any graphics context init, like opengl etension loading or whatever metal needs, maybe CoreGraphics for software renderer, depends

    // TODO: Fill in a barebones program menu

    // Trigger finishlanching

    // Allocate gameMemory

    // TODO: Init CoreAudio using device callback for now


    while (isRunning)
    {
        NSEvent* event;

        do {
            event = [NSApp nextEventMatchingMask:NSEventMaskAny
                untilDate:nil
                inMode:NSDefaultRunLoopMode
                dequeue:YES];

            switch ([event type])
            {
                default:
                    [NSApp sendEvent:event];
            }
        
        } while (event != nil);

        // Update input not set by the event loop (Mouse position relative to window/view?)
        // Poll HID/Controllers

        // TODO: Update and render the game
        // Wait for frame elapse
        // swap buffers on vblank?
    }

    return 0;
}
