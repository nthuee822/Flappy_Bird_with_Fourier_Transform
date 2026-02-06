#ifndef TONEDET_H
#define TONEDET_H

float Approx_FFT(int in[], int N, float Frequency);
void printNote(int k);
int fastRSS(int a, int b);
int fast_sine(int Amp, int th);
int fast_cosine(int Amp, int th);

//---------------------------------lookup data------------------------------------//
byte isin_data[128] = {
  0,   1,   3,   4,   5,   6,   8,   9,   10,  11,  13,  14,  15,  17,  18,  19,  20,
  22,  23,  24,  26,  27,  28,  29,  31,  32,  33,  35,  36,  37,  39,  40,  41,  42,
  44,  45,  46,  48,  49,  50,  52,  53,  54,  56,  57,  59,  60,  61,  63,  64,  65,
  67,  68,  70,  71,  72,  74,  75,  77,  78,  80,  81,  82,  84,  85,  87,  88,  90,
  91,  93,  94,  96,  97,  99,  100, 102, 104, 105, 107, 108, 110, 112, 113, 115, 117,
  118, 120, 122, 124, 125, 127, 129, 131, 133, 134, 136, 138, 140, 142, 144, 146, 148,
  150, 152, 155, 157, 159, 161, 164, 166, 169, 171, 174, 176, 179, 182, 185, 188, 191,
  195, 198, 202, 206, 210, 215, 221, 227, 236
};
unsigned int Pow2[14] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096};
byte RSSdata[20] = {7, 6, 6, 5, 5, 5, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2};
//---------------------------------------------------------------------------------//

// --- Global Configuration ---
#define SAMPLE_SIZE 128
#define MIC_PIN PA0
#define CENTER_VOLTAGE 512// 512 for arduino, 1024 for pico pi 2

int  input_buffer[SAMPLE_SIZE];
byte Note_Votage[13]={8,23,40,57,76,95,116,138,162,187,213,241,255}; //data for note detection based on frequency
float f_peaks[5]; // top 5 frequencies peaks in descending order
int noise_floor = 20;

static int sound_amp;

float Tone_det()
{ 
  long unsigned int total_time;
  int value;
  double sum = 0;
  double sum_square = 0;
  float sampling_freq;
  unsigned long start_time = micros();
  for(int i = 0; i < SAMPLE_SIZE; i++) {
    value = analogRead(MIC_PIN) - CENTER_VOLTAGE;     //rough zero shift
    //utilising time between two sample for windowing & amplitude calculation
    // Serial.println(value);
    sum = sum + value;              //to average value
    sum_square = sum_square + value * value;            // to RMS value
    value = value * (sin(i * PI / SAMPLE_SIZE) * sin(i * PI / SAMPLE_SIZE));   // Hann window
    input_buffer[i] = 10 * value;                // scaling for float to int conversion
    delayMicroseconds(200);   // based on operation frequency range, dT越大dF越小
  }
  unsigned long end_time = micros();

  sum = sum / SAMPLE_SIZE;               // Average amplitude
  sum_square = sqrt(sum_square / SAMPLE_SIZE);         // RMS amplitude
  sampling_freq =  (SAMPLE_SIZE * 1000000.0)/(end_time-start_time);  // real time sampling frequency 越小dF越小, e+6是因為microsec
  //for very low or no amplitude, this code won't start
  //it takes very small aplitude of sound to initiate for value sum2-sum1>3, 
  //change sum2-sum1 threshold based on requirement
  Serial.println("======= wow =======");
  Serial.println(sum);
  Serial.println(sum_square);
  Serial.println(sum_square - sum);
  Serial.println("===================");
  sound_amp = int(sum_square - sum);
  if(sum_square - sum > noise_floor){ 
    //EasyFFT based optimised FFT code, 
    //this code updates f_peaks array with 5 most dominent frequency in descending order
    // Serial.print("Sampling frequency: ");
    // Serial.println(sampling_freq);
    float f = Approx_FFT(input_buffer, SAMPLE_SIZE, sampling_freq);
    //Serial.print("Detected Frequency: ");
    //Serial.println(int(f));
    float fr = f;
    if(f>1040) f=0;
    if(f>=65.4   && f<=130.8) {f=255*((f/65.4)-1);}
    if(f>=130.8  && f<=261.6) {f=255*((f/130.8)-1);}
    if(f>=261.6  && f<=523.25){f=255*((f/261.6)-1);}
    if(f>=523.25 && f<=1046)  {f=255*((f/523.25)-1);}
    if(f>=1046 && f<=2093)  {f=255*((f/1046)-1);}
    if(f>255) f=254;

    // convert f to note
    int j = 1;
    int k = 0;
    int note = 0;
    // Serial.println(f);
    while(j == 1){
      if(f < Note_Votage[k]) {
        note = k;
        j = 0;
      }
      k++;  // a note with max peaks (harmonic) with aplitude priority is selected
      if(k == 13) { // The higher C note
        note = 0;
        j = 0;
      }
    }
    total_time = micros(); // time check
    //Serial.print("Total process time: ");
    //Serial.println(total_time - start_time);
    printNote(note);
    return fr;
  }
  return -1.0;
}

int get_sound_amp(){
  return sound_amp;
}

//-----------------------------PrintNote Function----------------------------------------------//
// Print note based on Tone detection result
void printNote(int k)
{
  // Serial.println(k);
  const char* notes[] =
  {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"}; // 0 - 11
  Serial.print("Note: ");
  Serial.println(notes[k]);
}


//-----------------------------FFT Function----------------------------------------------//
/*
Code to perform High speed and Accurate FFT on arduino,
setup:

1. in[]     : Data array, 
2. N        : Number of sample (recommended sample size 2,4,8,16,32,64,128,256,512...)
3. Frequency: sampling frequency required as input (Hz)

It will by default return frequency with max aplitude,
if you need complex output or magnitudes uncomment required sections

If sample size is not in power of 2 it will be clipped to lower side of number. 
i.e, for 150 number of samples, code will consider first 128 sample, remaining sample  will be omitted.
For Arduino nano, FFT of more than 256 sample not possible due to mamory limitation 
Code by ABHILASH
Contact: abhilashpatel121@gmail.com
Documentation & details: https://www.instructables.com/member/abhilash_patel/instructables/
*/

float Approx_FFT(int in[], int N, float Frequency)
{
  int a, c1, f, o, x, data_max, data_min = 0;
  long data_avg, data_mag, temp11;
  byte scale, check = 0;

  data_max = 0;
  data_avg = 0;
  data_min = 0;

  for (int i = 0; i < 12; i++) {
    if (Pow2[i] <= N) { o = i; }
  }
  a = Pow2[o];
  int out_r[a];
  int out_im[a];

  for (int i = 0; i < a; i++) {
    out_r[i] = 0;
    out_im[i] = 0;
    data_avg = data_avg + in[i];
    if (in[i] > data_max) { data_max = in[i]; }
    if (in[i] < data_min) { data_min = in[i]; }
  }

  data_avg = data_avg >> o;
  scale = 0;
  data_mag = data_max - data_min;
  temp11 = data_mag;

  if (data_mag > 1024) {
    while (temp11 > 1024) {
      temp11 = temp11 >> 1;
      scale = scale + 1;
    }
  }

  if (data_mag < 1024) {
    while (temp11 < 1024) {
      temp11 = temp11 << 1;
      scale = scale + 1;
    }
  }

  if (data_mag > 1024) {
    for (int i = 0; i < a; i++) {
      in[i] = in[i] - data_avg;
      in[i] = in[i] >> scale;
    }
    scale = 128 - scale;
  }

  if (data_mag < 1024) {
    scale = scale - 1;
    for (int i = 0; i < a; i++) {
      in[i] = in[i] - data_avg;
      in[i] = in[i] << scale;
    }
    scale = 128 + scale;
  }

  x = 0;
  for (int b = 0; b < o; b++) {
    c1 = Pow2[b];
    f = Pow2[o] / (c1 + c1);
    for (int j = 0; j < c1; j++) {
      x = x + 1;
      out_im[x] = out_im[j] + f;
    }
  }

  for (int i = 0; i < a; i++) {
    out_r[i] = in[out_im[i]];
    out_im[i] = 0;
  }

  int i10, i11, n1, tr, ti;
  float e;
  int c, s, temp4;
  for (int i = 0; i < o; i++) {
    i10 = Pow2[i];
    i11 = Pow2[o] / Pow2[i + 1];
    e = 1024 / Pow2[i + 1];
    e = 0 - e;
    n1 = 0;

    for (int j = 0; j < i10; j++) {
      c = e * j;
      while (c < 0) { c = c + 1024; }
      while (c > 1024) { c = c - 1024; }
      n1 = j;

      for (int k = 0; k < i11; k++) {
        temp4 = i10 + n1;
        if (c == 0) {
          tr = out_r[temp4];
          ti = out_im[temp4];
        } else if (c == 256) {
          tr = -out_im[temp4];
          ti = out_r[temp4];
        } else if (c == 512) {
          tr = -out_r[temp4];
          ti = -out_im[temp4];
        } else if (c == 768) {
          tr = out_im[temp4];
          ti = -out_r[temp4];
        } else if (c == 1024) {
          tr = out_r[temp4];
          ti = out_im[temp4];
        } else {
          tr = fast_cosine(out_r[temp4], c) - fast_sine(out_im[temp4], c);
          ti = fast_sine(out_r[temp4], c) + fast_cosine(out_im[temp4], c);
        }

        out_r[n1 + i10] = out_r[n1] - tr;
        out_r[n1] = out_r[n1] + tr;
        if (out_r[n1] > 15000 || out_r[n1] < -15000) { check = 1; }

        out_im[n1 + i10] = out_im[n1] - ti;
        out_im[n1] = out_im[n1] + ti;
        if (out_im[n1] > 15000 || out_im[n1] < -15000) { check = 1; }

        n1 = n1 + i10 + i10;
      }
    }

    if (check == 1) {
      for (int i = 0; i < a; i++) {
        out_r[i] = out_r[i] >> 1;
        out_im[i] = out_im[i] >> 1;
      }
      check = 0;
      scale = scale - 1;
    }
  }

  if (scale > 128) {
    scale = scale - 128;
    for (int i = 0; i < a; i++) {
      out_r[i] = out_r[i] >> scale;
      out_im[i] = out_im[i] >> scale;
    }
    scale = 0;
  } else {
    scale = 128 - scale;
  }

  int fout, fm, fstp;
  float fstep;
  fstep = Frequency / N;
  fstp = fstep;
  fout = 0;
  fm = 0;

  for (int i = 1; i < Pow2[o - 1]; i++) {
    out_r[i] = fastRSS(out_r[i], out_im[i]);
    out_im[i] = out_im[i - 1] + fstp;
    if (fout < out_r[i]) {
      fm = i;
      fout = out_r[i];
    }
  }

  float fa, fb, fc;
  fa = out_r[fm - 1];
  fb = out_r[fm];
  fc = out_r[fm + 1];
  fstep = (fa * (fm - 1) + fb * fm + fc * (fm + 1)) / (fa + fb + fc);

  return (fstep * Frequency / N);
}

//---------------------------------fast sine/cosine---------------------------------------//

int fast_sine(int Amp, int th)
{
  int temp3, m1, m2;
  byte temp1, temp2, test, quad, accuracy;
  accuracy = 5; 
  while (th > 1024) { th = th - 1024; }
  while (th < 0) { th = th + 1024; }
  quad = th >> 8;

  if (quad == 1)      { th = 512 - th; }
  else if (quad == 2) { th = th - 512; }
  else if (quad == 3) { th = 1024 - th; }

  temp1 = 0;
  temp2 = 128;
  m1 = 0;
  m2 = Amp;
  temp3 = (m1 + m2) >> 1;
  Amp = temp3;

  for (int i = 0; i < accuracy; i++) {
    test = (temp1 + temp2) >> 1;
    temp3 = temp3 >> 1;
    if (th > isin_data[test])      { temp1 = test; Amp = Amp + temp3; m1 = Amp; }
    else if (th < isin_data[test]) { temp2 = test; Amp = Amp - temp3; m2 = Amp; }
  }

  if (quad == 2)      { Amp = 0 - Amp; }
  else if (quad == 3) { Amp = 0 - Amp; }
  return (Amp);
}

int fast_cosine(int Amp, int th)
{
  th = 256 - th;
  return (fast_sine(Amp, th));
}

//--------------------------------------------------------------------------------//


//--------------------------------Fast RSS----------------------------------------//
int fastRSS(int a, int b)
{
  if (a == 0 && b == 0) { return (0); }
  int min, max, temp1, temp2;
  byte clevel;
  if (a < 0) a = -a;
  if (b < 0) b = -b;
  clevel = 0;
  if (a > b) { max = a; min = b; } else { max = b; min = a; }

  if (max > (min + min + min)) {
    return max;
  } else {
    temp1 = min >> 3;
    if (temp1 == 0) temp1 = 1;
    temp2 = min;
    while (temp2 < max) {
      temp2 = temp2 + temp1;
      clevel = clevel + 1;
    }
    temp2 = RSSdata[clevel];
    temp1 = temp1 >> 1;
    for (int i = 0; i < temp2; i++) { max = max + temp1; }
    return (max);
  }
}
//--------------------------------------------------------------------------------//


#endif