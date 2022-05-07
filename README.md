### VideoFrameStep

```
VideoFrameStep *player = new VideoFrameStep(@"./test.mov");

int w = player->width();
int h = player->height();
unsigned int *texture = new unsigned int[w*h];

for(int k=0; k<10; k++) {
	player->next(texture,w,h);
}

delete[] texture;
delere player;
```