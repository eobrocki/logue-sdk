#pragma once
/*
    BSD 3-Clause License

    Copyright (c) 2018, KORG INC.
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//*/


#include "userosc.h"
#include "biquad.hpp"
#include "../../inc/utils/float_math.h"


struct SineWaves 
{
    
    enum 
    {
        flag_none  = 0,
        flag_reset = 1   
    };
    
    enum waveShape 
    {
        sine = 0,
        tri,
        square,
        saw
    };
    
    
    
    struct Params 
    {
        
        float   osc2_vol;
        float   detune;
        float   distort;
        int8_t  coar_1;       //Coarse osc 1
        int8_t  coar_2;       //Coarse osc 2
        int8_t  id5_funct;    //Value determines what param id 5 modifies (i.e  0 : Attack Rate 1 : Decay Rate) 
        
        
        Params(void) :  osc2_vol     (1.0f), 
                        detune       (0.0f), 
                        distort      (0.0f),
                        coar_1       (1), 
                        coar_2       (1), 
                        id5_funct    (0)
        {}
        
    };
    
    
    
    struct State 
    {
        
        float    phi0;         //Phase Accumulator
        float    phi1;
        float    w0;           //Angular Velocity/Increment Amount
        float    w1;
        float    detune;
        bool     isRouteSerial;
        bool     holdCarPitch;
        bool     coarChangeCar;
        bool     duophonic;
        int8_t   initNoteVal;
        uint32_t carWavShap;
        uint32_t modWavShap;
        bool     envType;
        float    lfo;  
        float    lfoz;
        
        //Dexed Env Variables
        int32_t  level_;
        int32_t  oldLevel_;
        int      outlevel_;
        int      targetlevel_;
        bool     rising_;
        int      ix_;
        int      inc_;
        int      staticcount_;
        int      dexedAttRate;
        int      dexedDecRate;
        int      dexedRelRate;
        int      dexedAttLevel;
        int      dexedDecLevel;
        int      maxEnvVal;     // Set by Shape param, determines max amt of env applied to MOD OP
        int32_t  fb_buffer[2];
        int      fb_shift;
        int      feedback;
        bool     down_;
        int      levels_[4];
        int      rates_[4];
        bool     refreshEnv;
        uint32_t sr_multiplier;
        uint32_t flags:2;
    
        
        
        State(void) :   phi0         (0.f), 
                        w0           (440.f * k_samplerate_recipf),
                        carWavShap   (sine),
                        phi1         (0.f),
                        w1           (440.f * k_samplerate_recipf), //angular freq in Radians per sample
                        modWavShap   (sine),
                        detune       (0.f),
                        isRouteSerial(true), 
                        holdCarPitch (true),
                        coarChangeCar(true),
                        duophonic    (false),
                        initNoteVal  (0),
                        lfo          (0.f),
                        lfoz         (0.f),
                        level_       (0),
                        targetlevel_ (0),
                        outlevel_    (0),
                        oldLevel_    (0),
                        rising_      (false),
                        ix_          (0),
                        inc_         (0),
                        staticcount_ (0),
                        dexedAttRate (99),
                        dexedDecRate (99),
                        dexedRelRate (99),
                        dexedAttLevel(99),
                        dexedDecLevel(0),
                        maxEnvVal    (0),
                        levels_      {0,0,89,0},
                        rates_       {99,99,99,99},
                        refreshEnv   (false),
                        envType      (true),
                        fb_buffer    {0,0},
                        fb_shift     (0),
                        feedback     (0),
                        sr_multiplier(1 << 24)
        
        {
            reset();
            init_sr(k_samplerate);
            initDexedEnv();
        }
        
        
        
        inline void reset(void)
        {
            phi0 = 0;
            phi1 = 0;
            lfo = lfoz;
        }
      
      

        /* ------------------------------------------------------------------------------------------
        * 
        * DEXED ENV IMPLEMENTATION
        * 
        * -------------------------------------------------------------------------------------------*/
        int levellut[20] = 
        { 
            0, 5, 9, 13, 17, 20, 23, 25, 27, 29, 31, 33, 35, 37, 39, 41, 42, 43, 45, 46 
        };
        
        
        
        int statics[77] = 
        {
            1764000, 1764000, 1411200, 1411200, 1190700, 1014300, 992250,
            882000, 705600, 705600, 584325, 507150, 502740, 441000, 418950,
            352800, 308700, 286650, 253575, 220500, 220500, 176400, 145530,
            145530, 125685, 110250, 110250, 88200, 88200, 74970, 61740,
            61740, 55125, 48510, 44100, 37485, 31311, 30870, 27562, 27562,
            22050, 18522, 17640, 15435, 14112, 13230, 11025, 9261, 9261, 7717,
            6615, 6615, 5512, 5512, 4410, 3969, 3969, 3439, 2866, 2690, 2249,
            1984, 1896, 1808, 1411, 1367, 1234, 1146, 926, 837, 837, 705,
            573, 573, 529, 441, 441
            // and so on, I stopped measuring after R=76 (needs to be double-checked anyway)
        };
        
        
        
        void init_sr(double sampleRate) //sr_multiplier only used when samplerate is not 44100
        {
            sr_multiplier = (44100.0 / sampleRate) * (1<<24);
            //sr_multiplier for 48000 = 15413228 truncate the fraction .3392
        }
        
        
        
        void initDexedEnv()
        {
        
            rates_[0] = dexedAttRate;
            rates_[1] = dexedDecRate;
            rates_[3] = dexedRelRate;
            
            levels_[0] = dexedAttLevel;
            levels_[1] = dexedDecLevel;
            
            outlevel_ = scaleOutputlevel(maxEnvVal);
            outlevel_ = outlevel_ > 127 ? 127 : outlevel_; 
            outlevel_ = outlevel_ << 5;

            level_ = 0;
            down_  = true;
            
            advance(0); //go to idle stage
            
            
        }
        
        
        
        //Update is called once per every OSC cycle
        void updateDexedEnv()
        {
            
            rates_[0] = dexedAttRate;
            rates_[1] = dexedDecRate;
            rates_[3] = dexedRelRate;
            
            levels_[0] = dexedAttLevel;
            levels_[1] = dexedDecLevel;
            
            outlevel_ = scaleOutputlevel(maxEnvVal);
            outlevel_ = outlevel_ > 127 ? 127 : outlevel_; 
            outlevel_ = outlevel_ << 5;

            if ( down_ ) 
            {
            
                // Update the sustain level instead when holding down a note
                
                
                // for now we simply reset ourselve at level 3
                int newlevel    = levels_[2];
                int actuallevel = scaleOutputlevel(newlevel) >> 1;
                actuallevel     = (actuallevel << 6) - 4256;
                actuallevel     = actuallevel < 16 ? 16 : actuallevel;
                targetlevel_    = actuallevel << 16;
                advance(2);
                
            }
            
        }
        
        
        //Called once per each smaple frame. 
        //Adds to the output of the env, until the level reaches a certain point, then calls the advance function
        int32_t getDexedSample(int32_t frames)
        {
            
            if (staticcount_) 
            {
                //staticcount_ -= 64; //N = 1 << 6
                staticcount_ -= frames;
                
                if (staticcount_ <= 0) 
                {
                   staticcount_ = 0;
                   advance(ix_ + 1); 
                }
                
            }
            // ix : 0 pre-attack
            //    : 1 attack
            //    : 2 decay
            //    : 3 sustain
            //    : 4 post-release
            if (ix_ < 3 || ((ix_ < 4) && !down_))   // Only increment/decrement level if in Idle, Att, or Decay, else return level repeatedly
            {
            
                if (rising_) 
                {
                    
                    //Q24 format
                    //So 8 bits for the integer value, and 24 bits for the fraction
                    //1716 << 16 = 0000 0110 . 1011 0100 0000 0000 0000 0000 Q24 format 
                    //           = 6.703125
                    const int jumptarget = 1716;
                    
                    if (level_ < (jumptarget << 16)) //Clip lowest level_ value to (1716 << 16 = 6.703125)
                    {
                        level_ = jumptarget << 16;
                    }
                                                                     
                    level_ += (((17 << 24) - level_) >> 24) * inc_;  // Inc is set everytime the advance_ function is called    
                                                                     // Based on the current level, the amount added is scaled by the inc_ 
                    if (level_ >= targetlevel_) 
                    {
                        level_ = targetlevel_;
                        advance(ix_ + 1);
                    }
                    
                }
                
                else if (staticcount_) // Don't continue to last else statement if staticcount_ is nonzero because the if(staticcount_) statement already ran and decremented
                {
                    ;
                }
                
                else  // !rising && !staticcount_ 
                {  
                    
                    level_ -= inc_; //There's no scaling of amount subtracted
                    
                    if (level_ <= targetlevel_) 
                    {
                        level_ = targetlevel_;
                        advance(ix_ + 1);
                    }
                    
                }
                
            }
                                
            return level_;      // Level is the env value that needs to be applied to the mod signal
                                // level_ is in Log base 2 format, can be added with SinLog OR take the exp2 to convert to linear
        }
 
 
 
        // Only called once per change in env stage
        void advance(int newix)
        {
            
            ix_ = newix;
    
            if (ix_ < 4)       
            {
            
                int newlevel    = levels_[ix_];    
                int actuallevel = scaleOutputlevel(newlevel) >> 1;          //"actualvalue" determined after scaling 
                                                                            //Assuming newlevel = 99; actuallevel = 127 >> 1 = 63 
                                                                                   
                actuallevel     =  (actuallevel << 6) + outlevel_ - 4256;   // 4256 = 0001 0000 1010 0000
                                                                            // actuallevel = 63 << 6 = 1111 1100 0000
                                                                            // 
                                                                            // outLevel_ <<= 5 is performed before sent to update function, 
                                                                            // so if set to 99 the outlevel_ var is 127 << 5 = 111111100000
                                                                            //             = 1111 1100 0000  
                                                                            //
                                                                            // so  1111 1100 0000 + 1111 1100 0000 - 0001 0000 1010 0000 = 1110 1110 0000
                                                                            // 1110 1110 0000 = 3808 is the max value of the actual level
                                                                            // 3808 << 16 = 0000 1110 . 1110 0000 0000 0000 0000 0000
                                                                            //            = 14.875 
                                                                         
                actuallevel  = actuallevel < 16 ? 16 : actuallevel;         //Clip the lowest level value possible to 16
                
                targetlevel_ = actuallevel << 16;  
                rising_      = (targetlevel_ > level_);                     // If newlevel is greater than the current output of the env

                // Convert the rate range from 0-99 to 0-63
                int qrate = (rates_[ix_] * 41) >> 6;                        // assuming 99 for rate value, 99 * 41 = 1111 1101 1011 
                                   
                if (targetlevel_ == level_) 
                {
                
                    // approximate number of samples at 44.100 kHz to achieve the time
                    // empirically gathered using 2 TF1s, could probably use some double-checking
                    // and cleanup, but it's pretty close for now.
                    int staticrate = rates_[ix_];
                    staticcount_   = staticrate < 77 ? statics[staticrate] : 20 * (99 - staticrate);    // So for rate of 99, it would be 0 // For rate = 78 : 20 * (99 - 77) = 440 
                    staticcount_   = (int) ( ((int64_t) staticcount_ * (int64_t) sr_multiplier) >> 24); // Max static count value 1764000(Q24) * sr_multiplier(Q24)
                  
                }
                
                else 
                {
                    staticcount_ = 0;
                }
                
                inc_ = (4 + (qrate & 3)) << (8 + (qrate >> 2)); // Assuming qrate = 63
                                                                // (4 + (111111 & 11)) << (8 + (111111 >> 2))
                                                                // (4 + 3) << 23 = 7 << 23 = 0011 . 1000 0000 0000 0000 0000 0000
                                                                //                         = 3.5 for increment
                // meh, this should be fixed elsewhere
                inc_ = (int)(((int64_t)inc_ * (int64_t)sr_multiplier) >> 24); 
                
            }
            
        }

        
        void keydown(bool d)
        {
            if (down_ != d)
            {
                down_ = d;  
                advance(d ? 0 : 3); //restart Env if note is down, Set to release if note is not down
            }
        }    
        
        int scaleOutputlevel(int outlevel)
        {
            //So max outlevel = 28 + 99 = 127.0
            return outlevel >= 20 ? 28 + outlevel : levellut[outlevel];
        }
        
        /* ------------------------------------------------------------------------------------------
        * 
        * END OF DEXED ENV IMPLEMENTATION
        * 
        * -------------------------------------------------------------------------------------------*/    
        
    };
  
    
    
    SineWaves(void) 
    { 
        init(); 
    }
    
    
    
    void init(void) 
    {     
        params = Params();
        state  = State();  
    }
    
    
    
    inline void updatePitch(float w0, float w1) 
    {
        state.w0 = w0;
        state.w1 = w1; 
    }
    
    
    
    inline void updateW0(float w0) 
    { 
        state.w0 = w0; 
    }
    
    
    
    inline void updateW1(float w1) 
    { 
        state.w1 = w1;
    }
    
    
    
    inline void updateWaveStates(int32_t status)
    {
     
        /* ----------------------------------------------------
         * 1 : Sine | Sine           5 : Saw  | Sine
         * 2 : Sine | Saw            6 : Saw  | Saw
         * 3 : Sine | Square         7 : Saw  | Square
         * 4 : Sine | Tri / Parabola 8 : Saw  | Tri / Parabola
         * ----------------------------------------------------
         * 9  : Square | Sine        13 : Tri | Sine
         * 10 : Square | Saw         14 : Tri | Saw
         * 11 : Square | Square      15 : Tri | Square
         * 12 : Square | Tri / Para  16 : Tri | Tri / Parabola
         * ----------------------------------------------------*/
        state.carWavShap = status / 4; 
        state.modWavShap = status % 4;  
        
    }
    
    
    State  state;
    Params params;
    dsp::BiQuad prelpf, postlpf;
};
