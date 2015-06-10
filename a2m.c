#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static unsigned short a2m_sixdepak(
    unsigned short*,
    unsigned char*,
    unsigned short);

#define ADPLUG_A2M_COPYRANGES		6
#define ADPLUG_A2M_FIRSTCODE		257
#define ADPLUG_A2M_MINCOPY		3
#define ADPLUG_A2M_MAXCOPY		255
#define ADPLUG_A2M_CODESPERRANGE	(ADPLUG_A2M_MAXCOPY - ADPLUG_A2M_MINCOPY + 1)
#define ADPLUG_A2M_MAXCHAR		(ADPLUG_A2M_FIRSTCODE + ADPLUG_A2M_COPYRANGES * ADPLUG_A2M_CODESPERRANGE - 1)
#define ADPLUG_A2M_TWICEMAX		(2 * ADPLUG_A2M_MAXCHAR + 1)

#define MAXFREQ (2000)
#define MINCOPY ADPLUG_A2M_MINCOPY
#define MAXCOPY ADPLUG_A2M_MAXCOPY
#define COPYRANGES ADPLUG_A2M_COPYRANGES
#define CODESPERRANGE ADPLUG_A2M_CODESPERRANGE
#define TERMINATE 256
#define FIRSTCODE ADPLUG_A2M_FIRSTCODE
#define MAXCHAR (FIRSTCODE + COPYRANGES * CODESPERRANGE - 1)
#define SUCCMAX (MAXCHAR + 1)
#define TWICEMAX ADPLUG_A2M_TWICEMAX
#define ROOT 1
#define MAXBUF (42 * 1024)
#define MAXDISTANCE 21389
#define MAXSIZE (21389 + MAXCOPY)

const unsigned short bitvalue[14] =
{1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192};

const short copybits[COPYRANGES] =
{4, 6, 8, 10, 12, 14};

const short copymin[COPYRANGES] =
{0, 16, 80, 336, 1360, 5456};

typedef struct {
  unsigned char data[11],arpstart,arpspeed,arppos,arpspdcnt,misc;
  signed char slide;
} Instrument;

Instrument* inst;

typedef struct {
  unsigned short freq,nextfreq;
  unsigned char oct,vol1,vol2,inst,fx,info1,info2,key,nextoct,
    note,portainfo,vibinfo1,vibinfo2,arppos,arpspdcnt;
  signed char trigger;
} Channel;

Channel* channel;

typedef struct {
  unsigned char note,command,inst,param2,param1;
} Tracks;

Tracks** tracks;

unsigned char *order, *arplist, *arpcmd, initspeed;
unsigned short tempo, **trackord, bpm, nop = 0;
unsigned long length, restartpos, activechan = 0xffffffff;
int flags = Standard;
char songname[43], author[43], instname[250][33];

unsigned short ibitcount, ibitbuffer, ibufcount, obufcount, input_size,
               output_size, leftc[ADPLUG_A2M_MAXCHAR+1], rghtc[ADPLUG_A2M_MAXCHAR+1],
               dad[ADPLUG_A2M_TWICEMAX+1], freq[ADPLUG_A2M_TWICEMAX+1], *wdbuf;
unsigned char *obuf, *buf;

unsigned char speed, del, songend, regbd;
unsigned short rows, notetable[12];
unsigned long rw, ord = 0, nrows = 0, npats = 0, nchans = 0;

void dealloc() {
  if(inst) {
    free(inst);
  }
  if(order) {
    free(order);
  }
  if(arplist) {
    free(arplist);
  }
  if(arpcmd) {
    free(arpcmd);
  }

  dealloc_patterns();
}

void dealloc_patterns() {
  unsigned long i;

  // dealloc everything previously allocated
  if(npats && nrows && nchans) {
    for(i=0;i<npats*nchans;i++) free(tracks[i]);
    free(tracks);
    for(i=0;i<npats;i++) free(trackord[i]);
    free(trackord);
    free(channel);
  }
}

int realloc_patterns(unsigned long pats, unsigned long rows, unsigned long chans) {
  unsigned long i;

  dealloc_patterns();

  // set new number of tracks, rows and channels
  npats = pats; nrows = rows; nchans = chans;

  // alloc new patterns
  //tracks = new Tracks *[pats * chans];
  tracks = (Tracks**)malloc(sizeof(size_t) * pats * chans);
  for(i=0;i<pats*chans;i++) {
    //tracks[i] = new Tracks[rows];
    tracks[i] = malloc(sizeof(Tracks) * rows);
  }
  //trackord = new unsigned short *[pats];
  trackord = malloc(sizeof(size_t) * pats);
  for(i=0;i<pats;i++) {
    //trackord[i] = new unsigned short[chans];
    trackord[i] = (unsigned short*)malloc(sizeof(unsigned short) * chans);
  }
  //channel = new Channel[chans];
  channel = (Channel*)malloc(sizeof(Channel) * chans);

  // initialize new patterns
  for(i=0;i<pats*chans;i++) memset(tracks[i],0,sizeof(Tracks)*rows);
  for(i=0;i<pats;i++) memset(trackord[i],0,chans*2);

  return 1;
}

int a2m_load(const char* filename) {
  FILE* f = fopen(filename, "r");

  if (f == NULL) {
    return 0;
  }

  char id[10];
  int i,j,k,t;
  unsigned int l;
  unsigned char *org, *orgptr, flags = 0, numpats, version;
  unsigned long crc, alength;
  unsigned short len[9], *secdata, *secptr;
  const unsigned char convfx[16] = {0,1,2,23,24,3,5,4,6,9,17,13,11,19,7,14};
  const unsigned char convinf1[16] = {0,1,2,6,7,8,9,4,5,3,10,11,12,13,14,15};
  const unsigned char newconvfx[] = {0,1,2,3,4,5,6,23,24,21,10,11,17,13,7,19,
    255,255,22,25,255,15,255,255,255,255,255,
    255,255,255,255,255,255,255,255,14,255};

  // read header
  crc = 0;
  //f->readString(id, 10); crc = f->readInt(4);
  fread(id, 10, 1, f);
  fread(&crc, 4, 1, f);
  //version = f->readInt(1); numpats = f->readInt(1);
  fread(&version, 1, 1, f);
  fread(&numpats, 1, 1, f);

  // file validation section
  if(strncmp(id,"_A2module_",10) || (version != 1 && version != 5 &&
        version != 4 && version != 8)) {
    //fp.close(f);
    fclose(f);
    return 0;
  }

  // load, depack & convert section
  nop = numpats; length = 128; restartpos = 0;
  if(version < 5) {
    for(i=0;i<5;i++) {
      // len[i] = f->readInt(2);
      fread(&len[i], 2, 1, f);
    }
    t = 9;
  } else {	// version >= 5
    for(i=0;i<9;i++) {
      // len[i] = f->readInt(2);
      fread(&len[i], 2, 1, f);
    }
    t = 18;
  }

  // block 0
  //secdata = new unsigned short [len[0] / 2];
  secdata = (unsigned short*)malloc(len[0]);
  if(version == 1 || version == 5) {
    for(i=0;i<len[0]/2;i++) {
      //secdata[i] = f->readInt(2);
      fread(&secdata[i], 2, 1, f);
    }
    //org = new unsigned char [MAXBUF]; orgptr = org;
    org = (unsigned char*)malloc(MAXBUF);
    orgptr = org;
    a2m_sixdepak(secdata,org,len[0]);
  } else {
    orgptr = (unsigned char *)secdata;
    for(i=0;i<len[0];i++) {
      //orgptr[i] = f->readInt(1);
      fread(&orgptr[i], 1, 1, f);
    }
  }
  memcpy(songname,orgptr,43); orgptr += 43;
  memcpy(author,orgptr,43); orgptr += 43;
  memcpy(instname,orgptr,250*33); orgptr += 250*33;

  for(i=0;i<250;i++) {	// instruments
    inst[i].data[0] = *(orgptr+i*13+10);
    inst[i].data[1] = *(orgptr+i*13);
    inst[i].data[2] = *(orgptr+i*13+1);
    inst[i].data[3] = *(orgptr+i*13+4);
    inst[i].data[4] = *(orgptr+i*13+5);
    inst[i].data[5] = *(orgptr+i*13+6);
    inst[i].data[6] = *(orgptr+i*13+7);
    inst[i].data[7] = *(orgptr+i*13+8);
    inst[i].data[8] = *(orgptr+i*13+9);
    inst[i].data[9] = *(orgptr+i*13+2);
    inst[i].data[10] = *(orgptr+i*13+3);

    if(version < 5)
      inst[i].misc = *(orgptr+i*13+11);
    else {	// version >= 5 -> OPL3 format
      int pan = *(orgptr+i*13+11);

      if(pan)
        inst[i].data[0] |= (pan & 3) << 4;	// set pan
      else
        inst[i].data[0] |= 48;			// enable both speakers
    }

    inst[i].slide = *(orgptr+i*13+12);
  }

  orgptr += 250*13;
  memcpy(order,orgptr,128); orgptr += 128;
  bpm = *orgptr; orgptr++;
  initspeed = *orgptr; orgptr++;
  if(version >= 5) flags = *orgptr;
  if(version == 1 || version == 5) {
    //delete [] org;
    free(org);
  }
  //delete [] secdata;
  free(secdata);

  // blocks 1-4 or 1-8
  alength = len[1];
  for(i = 0; i < (version < 5 ? numpats / 16 : numpats / 8); i++) {
    alength += len[i+2];
  }

  //secdata = new unsigned short [alength / 2];
  secdata = (unsigned short*)malloc(alength);
  if (version == 1 || version == 5) {
    for (l=0;l<alength/2;l++) {
      //secdata[l] = f->readInt(2);
      fread(&secdata[l], 2, 1, f);
    }
    //org = new unsigned char [MAXBUF * (numpats / (version == 1 ? 16 : 8) + 1)];
    org = (unsigned char*)malloc(MAXBUF * (numpats / (version == 1 ? 16 : 8) + 1));
    orgptr = org; secptr = secdata;
    orgptr += a2m_sixdepak(secptr,orgptr,len[1]); secptr += len[1] / 2;
    if (version == 1) {
      if (numpats > 16)
        orgptr += a2m_sixdepak(secptr,orgptr,len[2]); secptr += len[2] / 2;
      if (numpats > 32)
        orgptr += a2m_sixdepak(secptr,orgptr,len[3]); secptr += len[3] / 2;
      if (numpats > 48)
        a2m_sixdepak(secptr,orgptr,len[4]);
    }
    else {
      if (numpats > 8) {
        orgptr += a2m_sixdepak(secptr,orgptr,len[2]); secptr += len[2] / 2;
      }
      if(numpats > 16) {
        orgptr += a2m_sixdepak(secptr,orgptr,len[3]); secptr += len[3] / 2;
      }
      if(numpats > 24) {
        orgptr += a2m_sixdepak(secptr,orgptr,len[4]); secptr += len[4] / 2;
      }
      if(numpats > 32) {
        orgptr += a2m_sixdepak(secptr,orgptr,len[5]); secptr += len[5] / 2;
      }
      if(numpats > 40) {
        orgptr += a2m_sixdepak(secptr,orgptr,len[6]); secptr += len[6] / 2;
      }
      if(numpats > 48) {
        orgptr += a2m_sixdepak(secptr,orgptr,len[7]); secptr += len[7] / 2;
      }
      if(numpats > 56) {
        a2m_sixdepak(secptr,orgptr,len[8]);
      }
    }
    //delete [] secdata;
    free(secdata);
  }
  else {
    org = (unsigned char *)secdata;
    for(l=0;l<alength;l++) {
      //org[l] = f->readInt(1);
      fread(&org[l], 1, 1, f);
    }
  }

  if(version < 5) {
    for(i=0;i<numpats;i++) {
      for(j=0;j<64;j++) {
        for(k=0;k<9;k++) {
          Tracks* track = &tracks[i * 9 + k][j];
          unsigned char* o = &org[i*64*t*4+j*t*4+k*4];

          track->note = o[0] == 255 ? 127 : o[0];
          track->inst = o[1];
          track->command = convfx[o[2]];
          track->param2 = o[3] & 0x0f;
          if(track->command != 14)
            track->param1 = o[3] >> 4;
          else {
            track->param1 = convinf1[o[3] >> 4];
            if(track->param1 == 15 && !track->param2) {	// convert key-off
              track->command = 8;
              track->param1 = 0;
              track->param2 = 0;
            }
          }
          if(track->command == 14) {
            switch(track->param1) {
              case 2: // convert define waveform
                track->command = 25;
                track->param1 = track->param2;
                track->param2 = 0xf;
                break;
              case 8: // convert volume slide up
                track->command = 26;
                track->param1 = track->param2;
                track->param2 = 0;
                break;
              case 9: // convert volume slide down
                track->command = 26;
                track->param1 = 0;
                break;
            }
          }
        }
      }
    }
  }
  else {	// version >= 5
    realloc_patterns(64, 64, 18);

    for(i=0;i<numpats;i++) {
      for(j=0;j<18;j++) {
        for(k=0;k<64;k++) {
          struct Tracks	*track = &tracks[i * 18 + j][k];
          unsigned char	*o = &org[i*64*t*4+j*64*4+k*4];

          track->note = o[0] == 255 ? 127 : o[0];
          track->inst = o[1];
          track->command = newconvfx[o[2]];
          track->param1 = o[3] >> 4;
          track->param2 = o[3] & 0x0f;

          // Convert '&' command
          if(o[2] == 36) {
            switch(track->param1) {
              case 0:	// pattern delay (frames)
                track->command = 29;
                track->param1 = 0;
                // param2 already set correctly
                break;

              case 1:	// pattern delay (rows)
                track->command = 14;
                track->param1 = 8;
                // param2 already set correctly
                break;
            }
          }
        }
      }
    }
  }

  init_trackord();

  if(version == 1 || version == 5)
    //delete [] org;
    free(org);
  else
    //delete [] secdata;
    free(secdata);

  // Process flags
  if(version >= 5) {
    //CmodPlayer::flags |= Opl3;				// All versions >= 5 are OPL3
    //if(flags & 8) CmodPlayer::flags |= Tremolo;		// Tremolo depth
    //if(flags & 16) CmodPlayer::flags |= Vibrato;	// Vibrato depth
  }

  //fp.close(f);
  fclose(f);
  rewind(0);
  return 1;
}

float a2m_getrefresh() {
  if(tempo != 18) {
    return (float) (tempo);
  }
  else {
    return 18.2f;
  }
}

/*** private methods *************************************/

static void a2m_inittree() {
  unsigned short i;

  for(i=2;i<=TWICEMAX;i++) {
    dad[i] = i / 2;
    freq[i] = 1;
  }

  for(i=1;i<=MAXCHAR;i++) {
    leftc[i] = 2 * i;
    rghtc[i] = 2 * i + 1;
  }
}

static void a2m_updatefreq(unsigned short a, unsigned short b) {
  do {
    freq[dad[a]] = freq[a] + freq[b];
    a = dad[a];
    if(a != ROOT)
      if(leftc[dad[a]] == a)
        b = rghtc[dad[a]];
      else
        b = leftc[dad[a]];
  } while(a != ROOT);

  if(freq[ROOT] == MAXFREQ)
    for(a=1;a<=TWICEMAX;a++)
      freq[a] >>= 1;
}

static void a2m_updatemodel(unsigned short code) {
  unsigned short a=code+SUCCMAX,b,c,code1,code2;

  freq[a]++;
  if(dad[a] != ROOT) {
    code1 = dad[a];
    if(leftc[code1] == a)
      a2m_updatefreq(a,rghtc[code1]);
    else
      a2m_updatefreq(a,leftc[code1]);

    do {
      code2 = dad[code1];
      if(leftc[code2] == code1)
        b = rghtc[code2];
      else
        b = leftc[code2];

      if(freq[a] > freq[b]) {
        if(leftc[code2] == code1)
          rghtc[code2] = a;
        else
          leftc[code2] = a;

        if(leftc[code1] == a) {
          leftc[code1] = b;
          c = rghtc[code1];
        } else {
          rghtc[code1] = b;
          c = leftc[code1];
        }

        dad[b] = code1;
        dad[a] = code2;
        a2m_updatefreq(b,c);
        a = b;
      }

      a = dad[a];
      code1 = dad[a];
    } while(code1 != ROOT);
  }
}

static unsigned short a2m_inputcode(unsigned short bits) {
  unsigned short i,code=0;

  for(i=1;i<=bits;i++) {
    if(!ibitcount) {
      if(ibitcount == MAXBUF)
        ibufcount = 0;
      ibitbuffer = wdbuf[ibufcount];
      ibufcount++;
      ibitcount = 15;
    } else
      ibitcount--;

    if(ibitbuffer > 0x7fff)
      code |= bitvalue[i-1];
    ibitbuffer <<= 1;
  }

  return code;
}

static unsigned short a2m_uncompress() {
  unsigned short a=1;

  do {
    if(!ibitcount) {
      if(ibufcount == MAXBUF)
        ibufcount = 0;
      ibitbuffer = wdbuf[ibufcount];
      ibufcount++;
      ibitcount = 15;
    } else
      ibitcount--;

    if(ibitbuffer > 0x7fff)
      a = rghtc[a];
    else
      a = leftc[a];
    ibitbuffer <<= 1;
  } while(a <= MAXCHAR);

  a -= SUCCMAX;
  a2m_updatemodel(a);
  return a;
}

void a2m_decode() {
  unsigned short i,j,k,t,c,count=0,dist,len,index;

  a2n_inittree();
  c = a2m_uncompress();

  while(c != TERMINATE) {
    if(c < 256) {
      obuf[obufcount] = (unsigned char)c;
      obufcount++;
      if(obufcount == MAXBUF) {
        output_size = MAXBUF;
        obufcount = 0;
      }

      buf[count] = (unsigned char)c;
      count++;
      if(count == MAXSIZE)
        count = 0;
    } else {
      t = c - FIRSTCODE;
      index = t / CODESPERRANGE;
      len = t + MINCOPY - index * CODESPERRANGE;
      dist = a2m_inputcode(copybits[index]) + len + copymin[index];

      j = count;
      k = count - dist;
      if(count < dist)
        k += MAXSIZE;

      for(i=0;i<=len-1;i++) {
        obuf[obufcount] = buf[k];
        obufcount++;
        if(obufcount == MAXBUF) {
          output_size = MAXBUF;
          obufcount = 0;
        }

        buf[j] = buf[k];
        j++; k++;
        if(j == MAXSIZE) j = 0;
        if(k == MAXSIZE) k = 0;
      }

      count += len;
      if(count >= MAXSIZE)
        count -= MAXSIZE;
    }
    c = a2m_uncompress();
  }
  output_size = obufcount;
}

static unsigned short a2m_sixdepak(unsigned short* source,
    unsigned char*  dest,
    unsigned short  size) {
  if((unsigned int)size + 4096 > MAXBUF)
    return 0;

  //buf = new unsigned char [MAXSIZE];
  unsigned char* buf = (unsigned char*)malloc(MAXSIZE);
  input_size = size;
  ibitcount = 0; ibitbuffer = 0;
  obufcount = 0; ibufcount = 0;
  wdbuf = source; obuf = dest;

  a2m_decode();
  //delete [] buf;
  free(buf);
  return output_size;
}
