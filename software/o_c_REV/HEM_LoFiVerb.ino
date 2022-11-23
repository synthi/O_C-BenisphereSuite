// Copyright (c) 2018, Jason Justian
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#define HEM_LOFI_VERB_BUFFER_SIZE 307
#define HEM_LOFI_VERB_ALLPASS_SIZE 105

#define HEM_LOFI_VERB_SPEED 2 //cant be lower than 2 for memory reasons unless lofi echo is removed

#define LOFI_PCM2CV(S) ((uint32_t)S << 8) - 32512 //32767 is 128 << 8 32512 is 127 << 8 // 0-126 is neg, 127 is 0, 128-254 is pos

class LoFiVerb : public HemisphereApplet {
public:

    const char* applet_name() { // Maximum 10 characters
        return "LoFi Verb";
    }

    void Start() {
        countdown = HEM_LOFI_VERB_SPEED;
        for (int i = 0; i < HEM_LOFI_VERB_BUFFER_SIZE; i++) pcm[i] = 127; //char is unsigned in teensy (0-255)?   
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < HEM_LOFI_VERB_ALLPASS_SIZE; j++) allpass_pcm[i][j] = 127;
        }
        
        selected = 1; //for gui
    }

    void Controller() {
        play = !Gate(0); // Continuously play unless gated
        gated_record = Gate(1);

        countdown--;
        if (countdown == 0) {
            head++;
            if (head >= length) {
                head = 0;               
                //ClockOut(1);
            }
            
            ap_head++;
            if (ap_head >= ap_length) {
                ap_head = 0;               
               
            }

            for (int i = 0; i < 8; i++){ //for each of the 8 multitap heads; 
                int dt = multitap_heads[i] / HEM_LOFI_VERB_SPEED; //convert delaytime to length in samples 
                int writehead = (head+length + dt) % length; //have to add the extra length to keep modulo positive in case delaytime is neg   
                uint32_t tapeout = LOFI_PCM2CV(pcm[head]); // get the char out from the array and convert back to cv (de-offset)
                uint32_t feedbackmix = (min((tapeout * feedback / 100  + In(0)), cliplimit) + 32512) >> 8; //add to the feedback, offset and bitshift down
                pcm[writehead] = (char)feedbackmix;
            }

            //char ap = pcm[head]; //8 bit char of result of multitap 0 to 254            
            uint32_t ap_int = LOFI_PCM2CV(pcm[head]); //convert to signed full scale
            
            for (int i = 0; i < 4; i++){ //diffusors in series -- all done in 8 bit signed int
                uint32_t dry = ap_int;
                int dt = allpass_delay_times[i] / HEM_LOFI_VERB_SPEED; //delay time
                int writehead = (ap_head + ap_length + dt) % ap_length; //add delay time to get write location
                uint32_t tapeout = LOFI_PCM2CV(allpass_pcm[i][ap_head]);
                uint32_t feedbackmix = (min((tapeout * 50 / 100  + dry), cliplimit) + 32512) >> 8; //add to the feedback (50%), clip at 127 //buffer[bufidx] = input + (bufout*feedback);
                allpass_pcm[i][writehead] = (char)feedbackmix; 
                //int8_t invertedfb = feedbackmix * (-1); //invert signal (0-254 around "0" @ 127)
                //ap_int = max(ap_loclip, min((invertedfb / 2) + tapeout, ap_hiclip)); //output = -buffer[bufidx]*feedback + bufout
                uint32_t inv_dry = dry*(-1); 
                ap_int = tapeout + inv_dry;
            }                 

            //char ap = (char)ap_int;
            //uint32_t s = LOFI_PCM2CV(ap); //convert back to CV scale
            uint32_t s = ap_int;

            //uint32_t s = LOFI_PCM2CV(pcm[head]); 

            int SOS = In(1); // Sound-on-sound
            int live = Proportion(SOS, HEMISPHERE_MAX_CV, In(0)); //max_cv is 7680 scales vol. of live 
            int loop = play ? Proportion(HEMISPHERE_MAX_CV - SOS, HEMISPHERE_MAX_CV, s) : 0;
            Out(0, live + loop);
            countdown = HEM_LOFI_VERB_SPEED;
            
        }
    }

    void View() {
        gfxHeader(applet_name());
        DrawSelector();
        //DrawWaveform();
    }

    void OnButtonPress() {
        selected = 1 - selected;
        ResetCursor();
    }

    void OnEncoderMove(int direction) {
        //if (selected == 0) delaytime_pct = constrain(delaytime_pct += direction, 0, 99);
        if (selected == 1) feedback = constrain(feedback += direction, 0, 99);

        //amp_offset_cv = Proportion(amp_offset_pct, 100, HEMISPHERE_MAX_CV);
        //p[cursor] = constrain(p[cursor] += direction, 0, 100);

    
    }

    uint32_t OnDataRequest() {
        uint32_t data = 0;
        return data;
    }

    void OnDataReceive(uint32_t data) {
    }

protected:
    void SetHelp() {
        //                               "------------------" <-- Size Guide
        help[HEMISPHERE_HELP_DIGITALS] = "Gate 1=Pause 2=Rec";
        help[HEMISPHERE_HELP_CVS]      = "1=Audio 2=SOS";
        help[HEMISPHERE_HELP_OUTS]     = "A=Audio B=EOC Trg";
        help[HEMISPHERE_HELP_ENCODER]  = "T=End Pt P=Rec";
        //                               "------------------" <-- Size Guide
    }
    
private:
    char pcm[HEM_LOFI_VERB_BUFFER_SIZE];
    uint16_t multitap_heads[8] = {438,613,565,538,484,514,450,422}; //adapted for 16.7khz
    uint32_t allpass_delay_times[4] = {85,210,167,129}; //adapted for 16.7khz
    char allpass_pcm[4][HEM_LOFI_VERB_ALLPASS_SIZE]; //4 buffers of 105 samples each
    bool record = 0; // Record always on
    bool gated_record = 0; // Record gated via digital in
    bool play = 0; //play always on
    int head = 0; // Locatioon of delay head
    int ap_head = 0; // Locatioon of allpass head

    //int delaytime_pct = 50; //delaytime as percentage of delayline buffer
    int feedback = 80;
    int countdown = HEM_LOFI_VERB_SPEED;
    int length = HEM_LOFI_VERB_BUFFER_SIZE;
    int ap_length = HEM_LOFI_VERB_ALLPASS_SIZE;
    uint32_t cliplimit = 32512;
    int8_t ap_hiclip = 127;
    int8_t ap_loclip = -127;
    int selected; //for gui
     
    
    void DrawWaveform() {
        int inc = HEM_LOFI_VERB_BUFFER_SIZE / 256;
        int disp[32];
        int high = 1;
        int pos = head - (inc * 15) - random(1,3); // Try to center the head
        if (head < 0) head += length;
        for (int i = 0; i < 32; i++)
        {
            int v = (int)pcm[pos] - 127;
            if (v < 0) v = 0;
            if (v > high) high = v;
            pos += inc;
            if (pos >= HEM_LOFI_VERB_BUFFER_SIZE) pos -= length;
            disp[i] = v;
        }
        
        for (int x = 0; x < 32; x++)
        {
            int height = Proportion(disp[x], high, 30);
            int margin = (32 - height) / 2;
            gfxLine(x * 2, 30 + margin, x * 2, height + 30 + margin);
        }
    }
    

    
    void DrawSelector()
    {
        for (int param = 0; param < 2; param++)
        {
            gfxPrint(31 * param, 15, param ? "Fb: " : "Ln: ");
            //gfxPrint(16, 15, delaytime_pct);
            gfxPrint(48, 15, feedback);
            if (param == selected) gfxCursor(0 + (31 * param), 23, 30);
        }
    }
    
    
    
 /*   void DrawStop(int x, int y) {
        if (record || play || gated_record) gfxFrame(x, y, 11, 11);
        else gfxRect(x, y, 11, 11);
    }
    
    void DrawPlay(int x, int y) {
        if (play) {
            for (int i = 0; i < 11; i += 2)
            {
                gfxLine(x + i, y + i/2, x + i, y + 10 - i/2);
                gfxLine(x + i + 1, y + i/2, x + i + 1, y + 10 - i/2);
            }
        } else {
            gfxLine(x, y, x, y + 10);
            gfxLine(x, y, x + 10, y + 5);
            gfxLine(x, y + 10, x + 10, y + 5);
        }
    }
    void DrawRecord(int x, int y) {
        gfxCircle(x + 5, y + 5, 5);
        if (record || gated_record) {
            for (int r = 1; r < 5; r++)
            {
                gfxCircle(x + 5, y + 5, r);
            }
        }
    }
*/    
    
};


////////////////////////////////////////////////////////////////////////////////
//// Hemisphere Applet Functions
///
///  Once you run the find-and-replace to make these refer to LoFiVerb,
///  it's usually not necessary to do anything with these functions. You
///  should prefer to handle things in the HemisphereApplet child class
///  above.
////////////////////////////////////////////////////////////////////////////////
LoFiVerb LoFiVerb_instance[2];

void LoFiVerb_Start(bool hemisphere) {
    LoFiVerb_instance[hemisphere].BaseStart(hemisphere);
}

void LoFiVerb_Controller(bool hemisphere, bool forwarding) {
    LoFiVerb_instance[hemisphere].BaseController(forwarding);
}

void LoFiVerb_View(bool hemisphere) {
    LoFiVerb_instance[hemisphere].BaseView();
}

void LoFiVerb_OnButtonPress(bool hemisphere) {
    LoFiVerb_instance[hemisphere].OnButtonPress();
}

void LoFiVerb_OnEncoderMove(bool hemisphere, int direction) {
    LoFiVerb_instance[hemisphere].OnEncoderMove(direction);
}

void LoFiVerb_ToggleHelpScreen(bool hemisphere) {
    LoFiVerb_instance[hemisphere].HelpScreen();
}

uint32_t LoFiVerb_OnDataRequest(bool hemisphere) {
    return LoFiVerb_instance[hemisphere].OnDataRequest();
}

void LoFiVerb_OnDataReceive(bool hemisphere, uint32_t data) {
    LoFiVerb_instance[hemisphere].OnDataReceive(data);
}
