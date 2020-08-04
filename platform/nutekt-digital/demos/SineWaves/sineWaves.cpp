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


#include "sineWaves.hpp"
#include "../../inc/userosc.h"
#include "../../inc/osc_api.h"
#include "../../inc/utils/float_math.h"
#include "../../inc/utils/fixed_math.h"

static SineWaves s_waves;



void OSC_INIT(uint32_t platform, uint32_t api)
{
  (void)platform;
  (void)api;
}



void OSC_CYCLE(const user_osc_param_t * const params,
               int32_t *yn,
               const uint32_t frames)
{
    
        SineWaves::State  &s = s_waves.state;
  const SineWaves::Params &p = s_waves.params; 
  
  {
       
      const uint32_t flags = s.flags;
      s.flags              = SineWaves::flag_none;
      
      
     /* -----------------------------------------------------------
      * DUOPHONIC MODE
      * 
      * TODO: Have the Phase of each OP sync when notes are changed, so notes don't go out of phase when pressing different keys
      * -----------------------------------------------------------*/ 
      if (s.duophonic)                                                 //Pitch is 16 bit : High Byte 0xffff (0-255): Low Bye 0xfff (0-255)
      {
        s_waves.updateW1(osc_w0f_for_note( (uint8_t) (( (params->pitch)>>8 ) + 12 * s_waves.params.coar_2), params->pitch & 0xFF));
        
        if (s.coarChangeCar)                                           //Update Pitch only on Coar_1 Param change
        {
            s_waves.updateW0(osc_w0f_for_note( (uint8_t) (( s_waves.state.initNoteVal ) + 12 * s_waves.params.coar_1), params->pitch & 0xFF));
            s_waves.state.coarChangeCar = false;
        }
        
      } 
      
     /* -----------------------------------------------------------
      * MONOPHONIC MODE
      * -----------------------------------------------------------*/ 
      else 
      {
         s_waves.updatePitch(osc_w0f_for_note( (uint8_t) (( (params->pitch)>>8 ) + 6 * p.coar_1), params->pitch & 0xFF),
                             osc_w0f_for_note( (uint8_t) (( (params->pitch)>>8 ) + 6 * p.coar_2), params->pitch & 0xFF));
      }
    

      // Should probably see about utilizing this more
      if (flags & SineWaves::flag_reset) 
          s.reset();     
      
      // Update ENV if dirty flag is set
      if (s.refreshEnv)
      {
          s.updateDexedEnv();
          s.fb_shift = s.feedback != 0 ? 8 - s.feedback : 16;
    
          s.refreshEnv = false;
      }
       
      s.lfo = q31_to_f32(params->shape_lfo);
      
  }
  
  
  /* -------------------------------------------------------------------
   * DEXED ENV and Feedback CALC
   * -------------------------------------------------------------------*/
  int32_t gain1     = s.oldLevel_;
  int32_t envOutInt = s.getDexedSample( (int32_t) frames ); 
  
  
  float envOutFlo = envOutInt * 0.00000006f; // 2^-24 = 0.00000006
  int32_t gain2   = int32_t ( exp2( envOutFlo - 14.0f ) * pow(2, 24) ) ; // Convert back to Q24
  s.oldLevel_     = gain2;                   //Set for next OSC Cycle

  int32_t dgain = (gain2 - gain1 + (int32_t) (frames >> 1)) / (int32_t) frames; // Didn't cast frames to int32_t before which caused the output to be pure noise, 
                                                                                // Remember to watch those datatypes kids!
  int32_t gain  = gain1;

  int32_t y0 = s.fb_buffer[0];
  int32_t y1 = s.fb_buffer[1];
  
  
  float phi0 = s.phi0;
  float phi1 = s.phi1;
  float det  = s.detune;
  
  float lfoz = s.lfoz;                            // Z indicates old value of LFO
  const float lfo_inc = (s.lfo - lfoz) / frames;  // Calc phase increment for LFO
  
  
  
  /* -------------------------------------------------------------------
    * DETUNE
    * TODO : Want to set the max detune to .5 of the note
    * TODO : Fix the buzzing on the detune. I believe it's a phase issue
    * -------------------------------------------------------------------*/
  float freqhz      = osc_notehzf((params->pitch)>>8);    
//   float detuneRatio = 0.0209f * fastexpf(-0.396f * ((freq / (1<<24)))) / 1024.0f;
  double detuneRatio = (0.000051728 * freqhz) / 15;            // The float was chosen because at +15 the max detune added is ~= 10 hz
  det               = detuneRatio * freqhz * (p.detune - 15); 
  det               *= k_samplerate_recipf;
    
  
  
// ****************************************************************************
// SAMPLE CYCLE
// ****************************************************************************
  
  q31_t * __restrict y = (q31_t *)yn;
  const q31_t * y_e    = y + frames;
  
  for (; y != y_e; )
  {
    
   /* -------------------------------------------------------------------
    * WAVESHAPE
    * 
    * TODO : Maybe morph between waveforms instead of switching?
    *
    * -------------------------------------------------------------------*/ 

    float (*waveTypeCar)(float x); 
    float (*waveTypeMod)(float x);
    
    switch (s.carWavShap)
    {
        case 0: waveTypeCar = osc_sinf; break;
        case 1: waveTypeCar = osc_sawf; break;
        case 2: waveTypeCar = osc_sqrf; break;
        case 3: waveTypeCar = osc_parf; break;
        default:waveTypeCar = osc_sinf; break;
    }    
    
    switch (s.modWavShap)
    {
        case 0: waveTypeMod = osc_sinf; break;
        case 1: waveTypeMod = osc_sawf; break;
        case 2: waveTypeMod = osc_sqrf; break;
        case 3: waveTypeMod = osc_parf; break;
        default:waveTypeMod = osc_sinf; break;
    }    
    
    
    
   /* -------------------------------------------------------------------
    * SIGNAL ROUTE
    * 
    * TODO : I want a way to quantize the input into the sin function
    * TODO : Implement the Shape LFO param to control the Volume of the Mod OP
    *        
    * -------------------------------------------------------------------*/ 
    float sig;
   
    gain             += dgain;
    double gainf      = (0.00000006f * gain);
    int32_t scaled_fb = (y0 + y1) >> (s.fb_shift + 1);
    
    y0 = y1;
    
    double scaled_fbf = scaled_fb * 0.00000006f;
    float lfoMix      = clipminmaxf(0.005f, 1.0f - lfoz, 0.995f);
    float modOpOut    = waveTypeMod(phi1 + scaled_fbf) * (gainf * lfoMix);
    
    y1 = int32_t( modOpOut * pow(2, 24) );
    
    if (s.isRouteSerial) 
        sig = 0.250f * waveTypeCar(phi0 + modOpOut);  //LFO shape can be added here. possibly (modOpOut * lfoz) + phi0
        
    else 
        sig = 0.250f * waveTypeCar(phi0) + 0.250f * (waveTypeMod(phi1) * modOpOut);
    
    
   /* -------------------------------------------------------------------
    * DISTORTION (No longer part of OSC anymore)
    * 
    * Maybe change the signal to a q31 then perform quantization operations
    * shift right by some amount, then shift left the same amount to quantize
    *  
    * Saturation functions that could be utilized
    * Try to find that DSP website with all of the example algos, 
    * search for distortion and Saturation algos
    *
    * sig = osc_sat_cubicf(sig);
    * sig = osc_sat_schetzenf(sig);
    * -------------------------------------------------------------------*/
    //     float dist_sig = si_roundf ( si_roundf (si_roundf (sig) + 0.5) );    
    //     sig            = (1.f - p.distort) * sig + (p.distort * dist_sig); /* Mix b/t the rounded signal and the orig */
    //     sig            = clip1m1f(sig);
    //     
    
    *(y++) = f32_to_q31(sig);
    
    phi0 += s.w0 + det;
    phi0 -= (uint32_t)phi0;
    phi1 += s.w1;
    phi1 -= (uint32_t)phi1;
    lfoz += lfo_inc;
  }
  
  s.fb_buffer[0] = y0;
  s.fb_buffer[1] = y1;
  
  s.phi0 = phi0;
  s.phi1 = phi1;
  s.lfo  = lfoz;
  
}



void OSC_NOTEON(const user_osc_param_t * const params)
{
   
    //Reset the state of the osc
    s_waves.state.flags |= SineWaves::flag_reset;
    
    //Dexed Env, reinititialize to attack
    s_waves.state.initDexedEnv();

    if (s_waves.state.holdCarPitch && s_waves.state.duophonic) //Check if hold Carrier Pitch is set to true by the note release function
    { 
        s_waves.state.initNoteVal = (params->pitch)>>8;  
        s_waves.updateW0(osc_w0f_for_note( (uint8_t) ((s_waves.state.initNoteVal) + 12 * s_waves.params.coar_1), params->pitch & 0xFF));
        s_waves.state.holdCarPitch = false;  
    }
    
}



void OSC_NOTEOFF(const user_osc_param_t * const params)
{
   
   (void)params;
   
   s_waves.state.keydown(false);
   
   if(s_waves.state.duophonic)
      s_waves.state.holdCarPitch = true;
   
}



void OSC_PARAM(uint16_t index, uint16_t value)
{ 
  
  /* --------------------------------------------------------------------------------------------------------
   * OSC Parameters : 
   * --------------------------------------------------------------------------------------------------------
   * SHAPE : Outlevel of the Modulator OP
   * --------------------------------------------------------------------------------------------------------
   * ALT : DETUNE 
   * --------------------------------------------------------------------------------------------------------
   * ID1 : CARRIER OP COARSE TUNE
   * --------------------------------------------------------------------------------------------------------
   * ID2 : MODULATOR OP COARSE TUNE
   * --------------------------------------------------------------------------------------------------------
   * ID3 : FEEDBACK AMT
   * --------------------------------------------------------------------------------------------------------
   * ID4 : OPS WAVE SHAPE 
   * --------------------------------------------------------------------------------------------------------
   * ID5 : Multi Param
   * --------------------------------------------------------------------------------------------------------
   * ID6 : Multi function    [1: Attack Rate  2: Decay Rate ]
   *                         [3: Attack Level 4: Decay Level]
   *                         [5: Release Rate 6: OP Route   ]
   * ----------------------------------------------------------------------------------------------------------*/                 
    
  SineWaves::Params &p = s_waves.params;
  SineWaves::State  &s = s_waves.state;
  
  switch (index) 
  {
  
    // COARSE TUNE FOR CARRIER 
    // ---------------------------------------------------------------------------
    case k_user_osc_param_id1: 
        
        p.coar_1 = value - 2;  // Minus 2 so the Lowest setting is an octave down from the current note 
        s.coarChangeCar = true;// Allows for the Coarse Tune to be changed during duophonic mode
        
        break;   
        
    
    // COARSE TUNE FOR MODULATOR
    // ----------------------------------------------------------------------------
    case k_user_osc_param_id2: 
        
        p.coar_2 = value - 2;  // Minus 2 so the Lowest setting is an octave down from the current note 
        
        break;
        
    
    // FEEDBACK       
    // ----------------------------------------------------------------------------    
    case k_user_osc_param_id3: 
        
        /*p.distort = value / 100.0f; // Knob value 0...100%  */  
        s.feedback = value;
        s.refreshEnv = true;
        
        break;
        
    
    // OPS WAVE SHAPE       
    // ---------------------------------------------------------------------------------    
    case k_user_osc_param_id4: 
        
        s_waves.updateWaveStates(value); // Knob value 0...15     
        
        break;
        
        
    // MULTI-PURPOSE KNOB
    // -----------------------------------------------------------------------------------    
    case k_user_osc_param_id5: 
         
        // I could try using splines for the curve so I can control the rate more finely 
        // Need to add more accuracy in the lower values, so quick attack can  be tuned               
        
        // Dexed Env
        // ----------------------------------------------------------------------------------
        if (p.id5_funct == 0)           // Att Rate
            s.dexedAttRate = value;
        
        else if (p.id5_funct == 1)      // Dec Rate
            s.dexedDecRate = value;
        
        else if (p.id5_funct == 2)      // Att level
            s.dexedAttLevel = value;
        
        else if (p.id5_funct == 3)
            s.dexedDecLevel = value;    // Dec level
            
        else if (p.id5_funct == 4)
            s.dexedRelRate = value;     // Rel Rate
        
        s.refreshEnv = true;
        
        
        if (p.id5_funct == 5)           // Set OP Route (In Parallel or Series)
        {
            if ( value > 50 )
                s.isRouteSerial = true;
            else 
                s.isRouteSerial = false;
        }
        
        break;
        
        
    // MULTI-PURPOSE KNOB Function
    // -----------------------------------------------------------------------------------        
    case k_user_osc_param_id6:
        
        //So 1 = Attack rate, 2 = Decay Rate, 3 =  
        p.id5_funct = value;
        
        break;
        
        
    case k_user_osc_param_shape: //MODULATOR VOLUME  
       
        p.osc2_vol = param_val_to_f32(value);
 
        s.maxEnvVal  =  (int)(param_val_to_f32(value) * 100.0f );
        s.refreshEnv = true;
        
        break;
    
        
    case k_user_osc_param_shiftshape: //CARRIER DETUNE 
                               
        p.detune = param_val_to_f32((value >> 5) - 15); // Reduce to 0-15
        
        break;
        
        
    default:
        
        break;
        
  }
    
}

