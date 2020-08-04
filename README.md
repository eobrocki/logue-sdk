# SineWaves Oscillator Plugin
This is a two Operator FM Synthesizer utilizing the Envelope, Detune, and Feedback code found in the DEXED vst by Suburban Digital (https://github.com/asb2m10/dexed).

# Features
## The parameters of the Oscillator are as follows <br/>

  - **Shape** : Modify the Modulator Operator's Output Level (Increases the amount of FM in Serial Mode)
  - **Alt**   : Detunes the Carrier Operator by a small amount
  
  - **Param1**: Carrier Operator Coarse Tune   (*in increments of 1/2 Octaves to allow for inharmonic tones*)
  - **Param2**: Modulator Operator Coarse Tune (*in increments of 1/2 Octaves to allow for inharmonic tones*)
  - **Param3**: Feedback Amount     (*only applies to Serial Mode*)
  - **Param4**: Operators Waveshape (*Choose between Sine, Saw, Square, and Parabolic wave shapes for each operator*)
  - **Param5**: The Multi-parameter knob (*Sets the parameter based on what the Multi Function knob is set to*)
  - **Param6**: The Multi-Function knob  (*Choose from an additional 6 parameters, and modify with the Multi-parameter knob*)

## Envelope ##
The Oscillator utilizes the Envelope code from the DEXED project. This applies only to the Modulator Operator. The Carrier operator is intended to use the bulit in Envelope on the Nutekt NTS-1.<br/>
Set the Envelope levels with the Multi-Function and Multi-Parameter Knobs<br/><br/>
    **Multi-Function** : <br/>
    1. Envelope Attack Rate <br/>
    2. Envelope Decay Rate <br/>
    3. Envelope Attack Level <br/>
    4. Envelope Decay Level <br/>
    5. Envelope Release Rate <br/>
    6. Operators Routing<br/><br/>
This way of adding parameters is a bit cumbersome, so I've decided 10 parameters total would be adequate for any Oscilators. <br/>
 
## Operator Routing ## 
Ability to route the Operators in Serial and Parallel modes, accessible as Param6 in the Multi-Function knob.
 - Serial mode   - Routes the output of the Modulator Operator to the input of the Carrier operator. This mode allows for FM synthesis.<br/>
 - Parallel mode - The outputs of each Operator are sent directly to the Main Audio Output. There is no FM possible with this routing.<br/>

<!-- ## Troubleshooting -->





