class VideoFrameStep {
    
    private:
    
        bool _isLoaded = false;
        bool _isFinished = false;
        bool _isLoop = false;
        
        float _percentage = 0;
        
        int _width = 0;
        int _height = 0;
        
        bool _isDuplicate = true;

        AVPlayerItemVideoOutput *_output;
        AVPlayerItem *_playerItem;
        AVPlayer *_player;
    
        dispatch_semaphore_t _semaphore = dispatch_semaphore_create(0);
    
        void wait() {
            [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:1.0/120.0]];
        }
    
    public:
    
        bool isLoaded() { return this->_isLoaded; }
        bool isFinished() { return this->_isLoaded; }
        
        int width() { return this->_width; }
        int height() { return this->_height; }

        float percentage() { return this->_percentage; }
    
        VideoFrameStep(NSString *filePath, bool isLoop=false) {
            
            this->_isLoop = isLoop;
            NSURL *fileURL = [NSURL fileURLWithPath:filePath];
            
            NSLog(@"%@",filePath);
            
            NSDictionary *options = @{(id)AVURLAssetPreferPreciseDurationAndTimingKey:@(YES)};
            AVURLAsset *asset = [AVURLAsset URLAssetWithURL:fileURL options:options];
            
            if(asset==nil) {
                NSLog(@"Error loading asset : %@", [fileURL description]);
            }
            else {
                
                [asset loadValuesAsynchronouslyForKeys:[NSArray arrayWithObject:@"tracks"] completionHandler:^{
                    
                    this->_width = asset.tracks[0].naturalSize.width;
                    this->_height = asset.tracks[0].naturalSize.height;
                    
                    NSError *error = nil;
                    AVKeyValueStatus status = [asset statusOfValueForKey:@"tracks" error:&error];
                    if(status==AVKeyValueStatusLoaded) {
                                                
                        NSDictionary *settings = @{ (id)kCVPixelBufferPixelFormatTypeKey : [NSNumber numberWithInt:kCVPixelFormatType_32BGRA] };
                        this->_output = [[AVPlayerItemVideoOutput alloc] initWithPixelBufferAttributes:settings];
                        
                        this->_playerItem = [AVPlayerItem playerItemWithAsset:asset];
                        [this->_playerItem addOutput:this->_output];
                        this->_player = [AVPlayer playerWithPlayerItem:this->_playerItem];
                        
                        [this->_player setMuted:YES];
                        [this->_player play];
                        
                        this->_isLoaded = true;
                                                
                    }
                    else {
                        NSLog(@"Failed to load the tracks : %ld (%@)",status,error);
                    }
                    
                    dispatch_semaphore_signal(this->_semaphore);
                }];
                
                dispatch_semaphore_wait(this->_semaphore,DISPATCH_TIME_FOREVER);
                
                if(this->_isLoaded) {
                    
                    while(true) {
                        if([[this->_player currentItem] status]==AVPlayerItemStatusReadyToPlay) break;
                        this->wait();
                    }
                }
                
                [this->_player pause];
            }
        }
    
        ~VideoFrameStep() {
        }
    
        void test() {
            CMTime duration = [[this->_player currentItem] duration];
            NSLog(@"%f",CMTimeGetSeconds(duration));
        }
            
        bool next(unsigned int *ptr, int w, int h, int rb=-1) {
            
            if(!this->_isLoaded||this->_isFinished) return false;
            
            if(rb<0) rb = w;

            while(true) {
                
                CMTime duration = [[this->_player currentItem] duration];
                CMTime current = [this->_output itemTimeForHostTime:CACurrentMediaTime()];
                                                
                if([this->_output hasNewPixelBufferForItemTime:current]) {
                    
                    CVPixelBufferRef buffer = [this->_output copyPixelBufferForItemTime:current itemTimeForDisplay:nil];
                    
                    if(buffer) {
                        
                        if(this->_percentage>=100) this->_percentage = 100;
                        
                        CVPixelBufferLockBaseAddress(buffer,0);
                        
                        unsigned int *baseAddress = (unsigned int *)CVPixelBufferGetBaseAddress(buffer);
                        size_t width = CVPixelBufferGetWidth(buffer);
                        size_t height = CVPixelBufferGetHeight(buffer);
                        int row = ((int)CVPixelBufferGetBytesPerRow(buffer))>>2;
                        
                        float sx = (width-1)/(float)(w-1);
                        float sy = (height-1)/(float)(h-1);
                    
                        for(int i=0; i<h; i++) {
                            
                            int i2 = i*sy;
                            
                            unsigned int *src = baseAddress+i2*row;
                            unsigned int *dst = ptr+i*rb;
                            
                            for(int j=0; j<w; j++) {
                                
                                int j2 = j*sx;
                                unsigned int argb = *(src+j2);
                                
                                *dst++ = (0xFF000000|(argb&0xFF)<<16|(argb&0xFF00)|((argb>>16)&0xFF));
                                
                            }
                        }
                        
                        CVPixelBufferUnlockBaseAddress(buffer,0);
                        CVPixelBufferRelease(buffer);
                        
                        break;
                        
                    }
                    else {
                        
                        CVPixelBufferUnlockBaseAddress(buffer,0);
                        CVPixelBufferRelease(buffer);
                        
                        this->wait();
                    }
                    
                }
                
                this->_percentage = (CMTimeGetSeconds(current)/CMTimeGetSeconds(duration))*100.0;

                if(this->_percentage>=100.0) {
                    this->_percentage = 100.0;
                    break;
                } 
                
            }
                        
            if(this->_percentage<100) {
                [this->_player.currentItem stepByCount:1];
            }
            else {
                this->_isFinished = true;
                if(this->_isLoop) {
                    [this->_player seekToTime:kCMTimeZero toleranceBefore:kCMTimeZero toleranceAfter:kCMTimeZero completionHandler:^(BOOL finished) {
                        this->_isFinished = false;
                    }];
                }
                return false;
            }
            
            return true;
        }
};
