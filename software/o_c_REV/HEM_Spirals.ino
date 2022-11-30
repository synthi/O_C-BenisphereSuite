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

#define MAX_STEPS 32
#define MAX_RESTS 8

const int cursor_pos[6][3] = {
    {13, 23, 25},   // scale
    {50, 33, 12},   // root
    {38, 43, 25},   // rotation
    {56, 53, 6},    // rests
    {1, 63, 8},     // lock icon
    {50, 63, 12}    // lock steps
};

class Spirals : public HemisphereApplet {
public:

    const char* applet_name() {
        return "Spirals";
    }

    void Start() {
        r = 0;
        setting_rotation = rotation = 0.62;

        num_rests = 4;
        scale_idx = 0;
        BuildNotes();

        locked = false;
        lock_steps = 8;
    }

    void Controller() {
        if (Clock(0)) {
            int play_note = Advance();
            if(play_note >= 0) {
                Out(0, play_note);
                ClockOut(1);
            }
        }

        if(Clock(1)) {
            ToggleLock();
            // reset rotation??
            //r = 0 - rotation;
            //lock_read = 0;
        }

        rotation = setting_rotation + (DetentedIn(0) * (0.5 / HEMISPHERE_MAX_CV));

        if(Changed(1) && In(1) >= 0) {
            uint8_t quantized = rootQuantizer.Process(In(1)) % 12;
            if(root != quantized) {
                root = quantized;
                BuildNotes();
            }
        }

    }

    void View() {
        gfxHeader(applet_name());
        DrawSetings();
    }

    void OnButtonPress() {
        if (++cur_setting > 5) cur_setting = 0;
    }

    void OnEncoderMove(int direction) {
        if (cur_setting == 0) { // Scale selection
            scale_idx += direction;
            if (scale_idx >= OC::Scales::NUM_SCALES) scale_idx = 0;
            if (scale_idx < 0) scale_idx = OC::Scales::NUM_SCALES - 1;
            BuildNotes();
        } else if(cur_setting == 1) { // Root selection
            root = constrain(root + direction, 0, 11);
            BuildNotes();
        } else if(cur_setting == 2) { // Rotation selection
            setting_rotation = constrain(setting_rotation + (0.01 * direction), 0.0, 1.0);
            rotation = setting_rotation;
        } else if(cur_setting == 3) { // Rests
            num_rests = constrain(num_rests + direction, 0, MAX_RESTS);
            BuildNotes();
        } else if(cur_setting == 4) { // Locked
            ToggleLock();
        } else if(cur_setting == 5) { // Lock Steps
            lock_steps = constrain(lock_steps + direction, 0, MAX_STEPS);
        }
    }
        
    uint32_t OnDataRequest() {
        uint32_t data = 0;
        // example: pack property_name at bit 0, with size of 8 bits
        // Pack(data, PackLocation {0,8}, property_name); 
        return data;
    }

    void OnDataReceive(uint32_t data) {
        // example: unpack value at bit 0 with size of 8 bits to property_name
        // property_name = Unpack(data, PackLocation {0,8}); 
    }

protected:
    void SetHelp() {
        //                               "------------------" <-- Size Guide
        help[HEMISPHERE_HELP_DIGITALS] = "Clk 1=BPM 2=LOCK";
        help[HEMISPHERE_HELP_CVS]      = "1=ROT 2=ROOT";
        help[HEMISPHERE_HELP_OUTS]     = "A=CV B=TRIG";
        help[HEMISPHERE_HELP_ENCODER]  = "T=Value P=Setting";
        //                               "------------------" <-- Size Guide
    }
    
private:
    int cur_setting;

    double rotation;
    double setting_rotation;
    double r;

    uint8_t root;
    OC::SemitoneQuantizer rootQuantizer;

    int notes[32];
    int num_notes;
    int num_rests;
    int scale_idx;

    bool locked;
    int lock_steps;
    int lock_write;
    int lock_read;
    int lock_start;
    int lock_end;
    double steps[MAX_STEPS];

    void ToggleLock() {
        locked = !locked;
        if(locked) {
            lock_start = lock_write - lock_steps;
            if(lock_start < 0) lock_start = MAX_STEPS + lock_start;
            lock_read = lock_start;

            lock_end = lock_start + lock_steps - 1;
            if(lock_end > MAX_STEPS) lock_end = lock_end - MAX_STEPS - 1;
        }
    }

    int Advance() {
        if(locked) {
            double d = steps[lock_read];
            if(lock_read == lock_end) {
                lock_read = lock_start;
            } else {
                lock_read++;
                if(lock_read >= MAX_STEPS) lock_read = 0;
            }

            return notes[(int)(d * num_notes)];
        } else {
            r = fmod(r + rotation * num_notes, num_notes);
            
            steps[lock_write] = r / num_notes;
            if(++lock_write >= MAX_STEPS) lock_write = 0;

            return notes[(int)r];
        }
    }

    void BuildNotes() {
        r = 0;
        braids::Scale scale = OC::Scales::GetScale(scale_idx);
        num_notes = scale.num_notes + num_rests;
        uint32_t euc = EuclideanPattern(num_notes, num_rests, 0);
        
        int n = 0;
        for(int i = 0; i < num_notes; i++) {
            if(euc & 1) {
                notes[i] = -1;
            } else {
                notes[i] = (root << 7) + scale.notes[n];
                n++;
            }
            euc = euc >> 1;
        }        
    }

    void gfxPrintDouble(double d) {
        int v = d * 100;
        bool neg = v < 0 ? 1 : 0;
        if (v < 0) v = -v;
        int wv = v / 100; // whole volts
        int dv = v - (wv * 100); // decimal
        gfxPrint(neg ? "-" : " ");
        gfxPrint(wv);
        gfxPrint(".");
        if (dv < 10) gfxPrint("0");
        gfxPrint(dv);
    }

    void DrawSetings() {
        gfxBitmap(1, 15, 8, SCALE_ICON);
        gfxPrint(12, 16, OC::scale_names_short[scale_idx]);

        gfxPrint(1, 24, "ROOT: ");
        gfxPrint(50, 24, OC::Strings::note_names_unpadded[root]);

        gfxPrint(1, 34, "ROT: ");
        gfxPos(30, 34);
        gfxPrintDouble(rotation);
        
        gfxPrint(1, 44, "RESTS: ");
        gfxPrint(56, 44, num_rests);

        if(locked) gfxBitmap(1, 54, 8, CHECK_ON_ICON);
        if(!locked) gfxBitmap(1, 54, 8, CHECK_OFF_ICON);

        gfxPrint(12, 54, "LOCK: ");
        gfxPrint(50, 54, lock_steps);

        gfxCursor(cursor_pos[cur_setting][0], cursor_pos[cur_setting][1], cursor_pos[cur_setting][2]);
    }
};


////////////////////////////////////////////////////////////////////////////////
//// Hemisphere Applet Functions
///
///  Once you run the find-and-replace to make these refer to Spirals,
///  it's usually not necessary to do anything with these functions. You
///  should prefer to handle things in the HemisphereApplet child class
///  above.
////////////////////////////////////////////////////////////////////////////////
Spirals Spirals_instance[2];

void Spirals_Start(bool hemisphere) {
    Spirals_instance[hemisphere].BaseStart(hemisphere);
}

void Spirals_Controller(bool hemisphere, bool forwarding) {
    Spirals_instance[hemisphere].BaseController(forwarding);
}

void Spirals_View(bool hemisphere) {
    Spirals_instance[hemisphere].BaseView();
}

void Spirals_OnButtonPress(bool hemisphere) {
    Spirals_instance[hemisphere].OnButtonPress();
}

void Spirals_OnEncoderMove(bool hemisphere, int direction) {
    Spirals_instance[hemisphere].OnEncoderMove(direction);
}

void Spirals_ToggleHelpScreen(bool hemisphere) {
    Spirals_instance[hemisphere].HelpScreen();
}

uint32_t Spirals_OnDataRequest(bool hemisphere) {
    return Spirals_instance[hemisphere].OnDataRequest();
}

void Spirals_OnDataReceive(bool hemisphere, uint32_t data) {
    Spirals_instance[hemisphere].OnDataReceive(data);
}
