/*
 * Copyright 2026 G.Pimblott
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "MacFileOpenHandler.h"
#import <Cocoa/Cocoa.h>
#include <iostream>

@interface FileEventHandler : NSObject
@property(nonatomic, assign) std::function<void(std::string)> callback;
@end

@implementation FileEventHandler

- (void)handleOpenDocuments:(NSAppleEventDescriptor *)event
             withReplyEvent:(NSAppleEventDescriptor *)replyEvent {
  NSAppleEventDescriptor *fileList =
      [event paramDescriptorForKeyword:keyDirectObject];
  NSInteger count = [fileList numberOfItems];

  // We only take the first file for now
  if (count > 0) {
    NSAppleEventDescriptor *descriptor = [fileList descriptorAtIndex:1];
    NSURL *url = [NSURL URLWithString:[descriptor stringValue]];
    // If not a file URL string, try fileURLWithPath if it's a path
    if (!url || ![url isFileURL]) {
      // Sometimes legacy events send paths? Modern creates URLs.
      // Try standard string processing
      NSString *pathString = [descriptor stringValue];
      if ([pathString hasPrefix:@"file://"]) {
        url = [NSURL URLWithString:pathString];
      } else {
        url = [NSURL fileURLWithPath:pathString];
      }
    }

    if (url && [url path]) {
      std::string path = [[url path] UTF8String];
      if (self.callback) {
        self.callback(path);
      }
    }
  }
}

@end

namespace platform {
namespace mac {

static FileEventHandler *g_handler = nil;

void installFileHandler(std::function<void(std::string)> callback) {
  if (!g_handler) {
    g_handler = [[FileEventHandler alloc] init];
  }
  g_handler.callback = callback;

  [[NSAppleEventManager sharedAppleEventManager]
      setEventHandler:g_handler
          andSelector:@selector(handleOpenDocuments:withReplyEvent:)
        forEventClass:kCoreEventClass
           andEventID:kAEOpenDocuments];
}

} // namespace mac
} // namespace platform
