// sha1.hpp ----------------------------------------------//
// -----------------------------------------------------------------------------

// Copyright 2013-2014 Albert Wei

// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt

// -----------------------------------------------------------------------------

// Revision History
// 04-29-2014 - Initial Release

// -----------------------------------------------------------------------------

#pragma once
#ifndef __MACHO_SHA1_INCLUDE__
#define __MACHO_SHA1_INCLUDE__


namespace macho{

#define MAXINPUTLEN 200

typedef struct {
    unsigned int length[2];
    unsigned int h[8];
    unsigned int w[80];
} sha;

class sha1{
public:
    enum {
        SHA1_DIGEST_LENGTH = 20,
        SHA1_BLOCK_SIZE = 64
    };
    sha1(){
        init(&_sha);
    }
    void update(std::string data);
    std::string finalize();
    
    static std::string hmac_sha1(std::string text, std::string key);
    static std::string _sha1(std::string text);
    static void transform(sha *sh);
    static void init(sha *sh);
    static void process(sha *sh,int byte);
    static void hash(sha *sh,char hash[20]);
protected:
    std::string _hmac_sha1(std::string text, std::string key);
private:
    sha         _sha;
};

#ifndef MACHO_HEADER_ONLY

#define H0 0x67452301L
#define H1 0xefcdab89L
#define H2 0x98badcfeL
#define H3 0x10325476L
#define H4 0xc3d2e1f0L

#define K0 0x5a827999L
#define K1 0x6ed9eba1L
#define K2 0x8f1bbcdcL
#define K3 0xca62c1d6L

#define PAD  0x80
#define ZERO 0

/* functions */

#define S(n,x) (((x)<<n) | ((x)>>(32-n)))

#define F0(x,y,z) ((x&y)|((~x)&z))
#define F1(x,y,z) (x^y^z)
#define F2(x,y,z) ((x&y) | (x&z)|(y&z)) 
#define F3(x,y,z) (x^y^z)

void sha1::transform(sha *sh){ 
    /* basic transformation step */
    unsigned int a,b,c,d,e,temp;
    int t;

    for (t=16;t<80;t++) sh->w[t]=S(1,sh->w[t-3]^sh->w[t-8]^sh->w[t-14]^sh->w[t-16]);
    a=sh->h[0]; b=sh->h[1]; c=sh->h[2]; d=sh->h[3]; e=sh->h[4];
    for (t=0;t<20;t++) { 
        /* 20 times - mush it up */
        temp=K0+F0(b,c,d)+S(5,a)+e+sh->w[t];
        e=d; d=c;
        c=S(30,b);
        b=a; a=temp;
    }
    for (t=20;t<40;t++) { 
        /* 20 more times - mush it up */
        temp=K1+F1(b,c,d)+S(5,a)+e+sh->w[t];
        e=d; d=c;
        c=S(30,b);
        b=a; a=temp;
    }
    for (t=40;t<60;t++) { 
        /* 20 more times - mush it up */
        temp=K2+F2(b,c,d)+S(5,a)+e+sh->w[t];
        e=d; d=c;
        c=S(30,b);
        b=a; a=temp;
    }
    for (t=60;t<80;t++) { 
        /* 20 more times - mush it up */
        temp=K3+F3(b,c,d)+S(5,a)+e+sh->w[t];
        e=d; d=c;
        c=S(30,b);
        b=a; a=temp;
    }
    sh->h[0]+=a; sh->h[1]+=b; sh->h[2]+=c;
    sh->h[3]+=d; sh->h[4]+=e;
} 

void sha1::init(sha *sh){ 
    /* re-initialise */
    int i;
    
    for (i=0;i<80;i++) sh->w[i]=0L;
    sh->length[0]=sh->length[1]=0L;
    
    sh->h[0]=H0;
    sh->h[1]=H1;
    sh->h[2]=H2;
    sh->h[3]=H3;
    sh->h[4]=H4;
}

void sha1::process(sha *sh,int byte){ 
    /* process the next message byte */
    int cnt;
    
    cnt=(int)((sh->length[0]/32)%16);
    
    sh->w[cnt]<<=8;
    sh->w[cnt]|=(unsigned int)(byte&0xFF);

    sh->length[0]+=8;
    if (sh->length[0]==0L) { sh->length[1]++; sh->length[0]=0L; }
    if ((sh->length[0]%512)==0) transform(sh);
}

void sha1::hash(sha *sh,char hash[20]){ 
    /* pad message and finish - supply digest */
    int i;
    unsigned int len0,len1;
    len0=sh->length[0];
    len1=sh->length[1];
    process(sh,PAD);
    while ((sh->length[0]%512)!=448) process(sh,ZERO);
    sh->w[14]=len1;
    sh->w[15]=len0;    
    transform(sh);
    for (i=0;i<20;i++){ 
        /* convert to bytes */
        hash[i]=((sh->h[i/4]>>(8*(3-i%4))) & 0xffL);
    }
    init(sh);
}

void sha1::update(std::string data){
    for (int i = 0; i < data.length(); i++)
        process(&_sha, data[i]);
}

std::string sha1::finalize(){
    char _hash[20] = { 0 };
    hash(&_sha, _hash);
   /* for (int i = 0; i < 20; i++){
        printf(" %02X", (unsigned char)_hash[i]);
    }*/
    return std::string(_hash, 20);
}

std::string sha1::hmac_sha1(std::string text, std::string key){
    sha1 sh;
    return sh._hmac_sha1(text, key);
}

std::string sha1::_hmac_sha1(std::string text, std::string key){
    std::string _key;
    std::string _buffer;
    BYTE        _ipad[64];
    BYTE        _opad[64];
    memset(_ipad, 0x36, sizeof(_ipad));
    memset(_opad, 0x5c, sizeof(_opad));
    if (key.length() > SHA1_BLOCK_SIZE){
        update(key);
        _key = finalize();
    }
    else{
        _key = key;
    }
    
    _key.resize(sizeof(_ipad));

    /* STEP 2 */
    for (int i = 0; i<sizeof(_ipad); i++)
        _ipad[i] ^= _key[i];

    /* STEP 3 */
    _buffer = std::string((char*)_ipad, 64);
    _buffer.append(text);

    /* STEP 4 */
    update(_buffer);
    std::string report = finalize();
    
    /* STEP 5 */
    for (int i = 0; i<sizeof(_opad); i++)
        _opad[i] ^= _key[i];
    /* STEP 6 */
    _buffer = std::string((char*)_opad, 64);
    _buffer.append(report);

    /*STEP 7 */
    update(_buffer);
    return finalize();
}

std::string sha1::_sha1(std::string text){
    sha1 sh;
    sh.update(text);
    return sh.finalize();
}

#endif

}; // namespace macho

#endif