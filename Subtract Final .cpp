#include <systemc.h>
#include <iostream>

// FloatingPointExtractor Module
SC_MODULE(FloatingPointExtractor) {
    sc_in<sc_uint<32>> a;
    sc_in<sc_uint<32>> b;
    sc_out<bool> a_sign;
    sc_out<sc_uint<8>> a_exp;
    sc_out<sc_uint<32>> a_significand;
    sc_out<bool> b_sign;
    sc_out<sc_uint<8>> b_exp;
    sc_out<sc_uint<32>> b_significand;
    sc_in<bool> clock;

    void extraction_process() {
        while (true) {
            wait();
         //Extraction
            bool a_sign0 = (a.read() & 0x80000000) >> 31;
            unsigned int a_exp0 = (a.read() & 0x7F800000) >> 23;
            unsigned int a_significand0 = (a.read() & 0x7FFFFF);

            bool b_sign0 = (b.read() & 0x80000000) >> 31;
            unsigned int b_exp0 = (b.read() & 0x7F800000) >> 23;
            unsigned int b_significand0 = (b.read() & 0x7FFFFF);

            unsigned int a_significand1 = (a_exp0 >= 1) ? (a_significand0 | (1 << 23)) : a_significand0;
            unsigned int b_significand1 = (b_exp0 >= 1) ? (b_significand0 | (1 << 23)) : b_significand0;

            unsigned int a_significand2 = (a_significand1 << 7);
            unsigned int b_significand2 = (b_significand1 << 7);

            unsigned int a_exp1 = ((a_exp0 == 0) ? 1 : a_exp0);
            unsigned int b_exp1 = ((b_exp0 == 0) ? 1 : b_exp0);
  //Special Cases
            if (a_exp0 == 255 && a_significand0 != 0) {
                a_sign.write(true);
                a_exp.write(0x7F);
                a_significand.write(0x7fffffff);
                b_sign.write(true);
                b_exp.write(0x7F);
                b_significand.write(0x7fffffff);
            } else if (b_exp0 == 255 && b_significand0 != 0) {
                a_sign.write(true);
                a_exp.write(0x7F);
                a_significand.write(0x7fffffff);
                b_sign.write(true);
                b_exp.write(0x7F);
                b_significand.write(0x7fffffff);
            } else if (a_exp0 == 255 && a_significand0 == 0 && b_exp0 == 255 && b_significand0 == 0 && a_sign0 != b_sign0) {
                a_sign.write(true);
                a_exp.write(0x7F);
                a_significand.write(0x7fffffff);
                b_sign.write(true);
                b_exp.write(0x7F);
                b_significand.write(0x7fffffff);
            } else if (a_exp0 == 255 && a_significand0 == 0) {
                a_sign.write(a_sign0);
                a_exp.write(0x7F);
                a_significand.write(0x7fffffff);
                b_sign.write(true);
                b_exp.write(0x7F);
                b_significand.write(0x7fffffff);
            } else if (b_exp0 == 255 && b_significand0 == 0) {
                a_sign.write(true);
                a_exp.write(0x7F);
                a_significand.write(0x7fffffff);
                b_sign.write(b_sign0);
                b_exp.write(0x7F);
                b_significand.write(0x7fffffff);
            } else {
                a_sign.write(a_sign0);
                a_exp.write(a_exp1);
                a_significand.write(a_significand2);
                b_sign.write(b_sign0);
                b_exp.write(b_exp1);
                b_significand.write(b_significand2);
            }
        }
    }
 //Constructor
    SC_CTOR(FloatingPointExtractor) : a("a"), b("b"), a_sign("a_sign"), a_exp("a_exp"), a_significand("a_significand"),
                                     b_sign("b_sign"), b_exp("b_exp"), b_significand("b_significand"), clock("clock") {
        SC_THREAD(extraction_process);
        sensitive << clock.pos();  //clock signal
    }
};

// FloatingPointSubtractor Module
SC_MODULE(FloatingPointSubtractor) {
    sc_in<bool> a_sign;
    sc_in<sc_uint<8>> a_exp;
    sc_in<sc_uint<32>> a_significand;
    sc_in<bool> b_sign;
    sc_in<sc_uint<8>> b_exp;
    sc_in<sc_uint<32>> b_significand;
    sc_out<bool> result_sign;
    sc_out<sc_uint<8>> result_exp;
    sc_out<sc_uint<32>> result_significand;
    sc_in<bool> clock;

    void subtraction_process() {
        while (true) {
            wait();

            unsigned int ans_exp;
            unsigned int ans_significand;
            bool ans_sign;
            unsigned int a_significand3 = a_significand.read();
            unsigned int b_significand3 = b_significand.read();
  //Exponent shifting
            if (a_exp.read() >= b_exp.read()) {
                unsigned int shift = a_exp.read() - b_exp.read();
                b_significand3 = (b_significand.read() >> ((shift > 31) ? 31 : shift));
                ans_exp = a_exp.read();
            } else {
                unsigned int shift = b_exp.read() - a_exp.read();
                a_significand3 = (a_significand.read() >> ((shift > 31) ? 31 : shift));
                ans_exp = b_exp.read();
            }
//Significand shifting and adding and sign change of second input
            if (a_sign.read() != b_sign.read()) {
                ans_significand = a_significand3 + b_significand3;
                ans_sign = a_sign.read();
            } else {
                if (a_significand3 >= b_significand3) {
                    ans_sign = a_sign.read();
                    ans_significand = a_significand3 - b_significand3;
                } else {
                    ans_sign = !a_sign.read();
                    ans_significand = b_significand3 - a_significand3;
                }
            }

            result_sign.write(ans_sign);
            result_exp.write(ans_exp);
            result_significand.write(ans_significand);
        }
    }

    SC_CTOR(FloatingPointSubtractor) : a_sign("a_sign"), a_exp("a_exp"), a_significand("a_significand"),
                                       b_sign("b_sign"), b_exp("b_exp"), b_significand("b_significand"),
                                       result_sign("result_sign"), result_exp("result_exp"),
                                       result_significand("result_significand"), clock("clock") {
        SC_THREAD(subtraction_process);
        sensitive << clock.pos();
    }
};

// FloatingPointNormaliser Module
SC_MODULE(FloatingPointNormaliser) {
    sc_in<bool> result_sign;
    sc_in<sc_uint<8>> result_exp;
    sc_in<sc_uint<32>> result_significand;
    sc_out<sc_uint<32>> nresult;
    sc_in<bool> clock;
    void normal_process() {
        while (true) {
            wait();

            unsigned int ans_exp = result_exp.read();
            unsigned int ans_significand = result_significand.read();
            bool ans_sign = result_sign.read();
    /* Normalization */
    int i;
    for (i=31; i>0 && ((ans_significand>>i) == 0); i-- ){;}
    
    if (i>23){

        //Rounding
        unsigned int twentyfourth = ((ans_significand&(1<<(i-23-1)))>>(i-23-1));

        unsigned int twentyfifth = 0;
        for(int j=0;j<i-23-1;j++){
            twentyfifth = twentyfifth | ((ans_significand & (1<<j))>>j);
        }

        if ((int(ans_exp) + (i-23) - 7) > 0 && (int(ans_exp) + (i-23) - 7) < 255){

            ans_significand = (ans_significand>>(i-23));

            ans_exp = ans_exp + (i-23) - 7;

            if (twentyfourth==1 && twentyfifth == 1){
        
                ans_significand += 1;

            }
            else if ((ans_significand&1)==1 && twentyfourth ==1 && twentyfifth == 0){
   
                ans_significand += 1;

            }

            if ((ans_significand>>24)==1){
                ans_significand = (ans_significand>>1);
                ans_exp += 1;

            }
        }

        //Overflow
        else if (int(ans_exp) + (i-23) - 7 >= 255){
            ans_significand = (1<<23);
            ans_exp = 255;
        }
}


    //When answer is zero
     if (i==0 && ans_exp < 255){
        ans_exp = 0;
    }
    
    /* Constructing floating point number from sign, exponent and significand */

    unsigned int ans = (ans_sign<<31) | (ans_exp<<23) | (ans_significand& (0x7FFFFF));
    nresult.write(ans);
    }
}


    SC_CTOR(FloatingPointNormaliser) {
        SC_THREAD(normal_process);
        sensitive << clock.pos();
    }
};


SC_MODULE(Top) {
    FloatingPointExtractor extractor;
    FloatingPointSubtractor subtractor;
    FloatingPointNormaliser normalization;
    sc_signal<bool> a_sign;
    sc_signal<sc_uint<8>> a_exp;
    sc_signal<sc_uint<32>> a_significands;
    sc_signal<bool> b_sign;
    sc_signal<sc_uint<8>> b_exp;
    sc_signal<sc_uint<32>> b_significands;
    sc_signal<bool> result_sign;
    sc_signal<sc_uint<8>> result_exp;
    sc_signal<sc_uint<32>> result_significand;
    sc_signal<sc_uint<32>> a;
    sc_signal<sc_uint<32>> b;
    sc_signal<sc_uint<32>> normalized_result;
    sc_clock clock;

    SC_CTOR(Top) : extractor("Extractor"), subtractor("Subtractor"), normalization("Normalization"), clock("clock", 1, SC_NS) {
        extractor.a(a);
        extractor.b(b);
        extractor.a_sign(a_sign);
        extractor.a_exp(a_exp);
        extractor.a_significand(a_significands);
        extractor.b_sign(b_sign);
        extractor.b_exp(b_exp);
        extractor.b_significand(b_significands);
        extractor.clock(clock);

        subtractor.a_sign(a_sign);
        subtractor.a_exp(a_exp);
        subtractor.a_significand(a_significands);
        subtractor.b_sign(b_sign);
        subtractor.b_exp(b_exp);
        subtractor.b_significand(b_significands);
        subtractor.result_sign(result_sign);
        subtractor.result_exp(result_exp);
        subtractor.result_significand(result_significand);
        subtractor.clock(clock);

        normalization.result_sign(result_sign);
        normalization.result_exp(result_exp);
        normalization.result_significand(result_significand);
        normalization.nresult(normalized_result);
        normalization.clock(clock);
    }
};

int sc_main(int argc, char* argv[]) {
    
    
        Top top("Top");

    sc_trace_file *tf = sc_create_vcd_trace_file("waveform");


    sc_trace(tf, top.clock, "clock");
    sc_trace(tf, top.a, "a");
    sc_trace(tf, top.b, "b");
    sc_trace(tf, top.a_sign, "a_sign");
    sc_trace(tf, top.a_exp, "a_exp");
    sc_trace(tf, top.a_significands, "a_significands");
    sc_trace(tf, top.b_sign, "b_sign");
    sc_trace(tf, top.b_exp, "b_exp");
    sc_trace(tf, top.b_significands, "b_significands");
    sc_trace(tf, top.result_sign, "result_sign");
    sc_trace(tf, top.result_exp, "result_exp");
    sc_trace(tf, top.result_significand, "result_significand");
    sc_trace(tf, top.normalized_result, "normalized_result");



    float a_float, b_float;
    cout << "Enter the value for a: ";
    cin >> a_float;
    cout << "Enter the value for b: ";
    cin >> b_float;

    unsigned int a_binary, b_binary;
    memcpy(&a_binary, &a_float, sizeof(a_binary));
    memcpy(&b_binary, &b_float, sizeof(b_binary));

    top.a.write(a_binary);
    top.b.write(b_binary);

    sc_start(10, SC_NS);

    unsigned int result = top.normalized_result.read();

    float result_float;
    memcpy(&result_float, &result, sizeof(result_float));

    cout << "Result: " << result_float << endl;

    sc_close_vcd_trace_file(tf);

    return 0;
}
