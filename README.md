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

[see also](https://gist.github.com/mizt/35d1e3485a7e1cc3530556bf0ce96095/)