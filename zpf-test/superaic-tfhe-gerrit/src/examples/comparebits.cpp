#include <iostream>
// #include <tfhe_core.h>
#include <tfhe.h>
#include<time.h>

using namespace std;

void compare_bits(LweSample *result, const LweSample *a, const LweSample *b, const LweSample *lsb_carry, LweSample *tmp, const TFheGateBootstrappingCloudKeySet *bk)
{
    bootsXNOR(tmp, a, b, bk);
    bootsMUX(result, tmp, lsb_carry, b, bk);
}

void minimum(LweSample *result, const LweSample *a, const LweSample *b, const int bits, const TFheGateBootstrappingCloudKeySet *bk)
{
    // find the minimum of a and b
    LweSample *tmps = new_gate_bootstrapping_ciphertext_array(2, bk->params);

    //initial the carry to 0
    bootsCONSTANT(&tmps[0], 0, bk);

    for (int i = 0; i < 16; i++)
        compare_bits(&tmps[0], &a[i], &b[i], &tmps[0], &tmps[1], bk);

    //tmps[0] = 0 represents a is larger, 1 b is larger
    //select the smaller one
    for (int i = 0; i < 16; i++)
        bootsMUX(&result[i], &tmps[0], &a[i], &b[i], bk);

    delete_gate_bootstrapping_ciphertext_array(2, tmps);
}

int main()
{
    const int minimum_lambda = 110;
    TFheGateBootstrappingParameterSet *params = new_default_gate_bootstrapping_parameters(minimum_lambda);

    //generate a random key
    uint32_t seed[] = {214, 1592, 657};
    tfhe_random_generator_setSeed(seed, 3);
    TFheGateBootstrappingSecretKeySet *key = new_random_gate_bootstrapping_secret_keyset(params);

    //do something
    int16_t plaintext1 = 10;
    int16_t plaintext2 = 2017;
    cout << "ALice: Hi there! Today, I will ask the cloud what is the minimum of " << plaintext1 << " and " << plaintext2 << endl;

    LweSample *ciphertext1 = new_gate_bootstrapping_ciphertext_array(16, params);
    LweSample *ciphertext2 = new_gate_bootstrapping_ciphertext_array(16, params);

    cout << " start to encrypt.... " << endl;
    for (int i = 0; i < 16; i++)
    {
        bootsSymEncrypt(&ciphertext1[i], (plaintext1 >> i) & 1, key);
        bootsSymEncrypt(&ciphertext2[i], (plaintext2 >> i) & 1, key);
    }

    cout<<"Cloud: Now , I will compute the smaller one homomorohically....."<<endl;
    //bootstapping key
    const TFheGateBootstrappingCloudKeySet *bk = &(key->cloud);
    LweSample *result = new_gate_bootstrapping_ciphertext_array(16, params);
    
    time_t t0 = clock();
    
    minimum(result, ciphertext1, ciphertext2, 16, bk);
    time_t t1 = clock();
    
    cout<<"compute the compare circuit in "<< (t1-t0)/CLOCKS_PER_SEC <<" secs"<<endl;

    int16_t answer = 0;
    for (int i = 0; i < 16; i++)
    {
        int ai = bootsSymDecrypt(&result[i], key);
        answer |= (ai << i);
    }
    cout << "answer: " << answer << endl;

    //delete all the pointers
    delete_gate_bootstrapping_secret_keyset(key);
    delete_gate_bootstrapping_parameters(params);
    delete_gate_bootstrapping_ciphertext_array(16, ciphertext1);
    delete_gate_bootstrapping_ciphertext_array(16, ciphertext2);
    delete_gate_bootstrapping_ciphertext_array(16, result);
}
