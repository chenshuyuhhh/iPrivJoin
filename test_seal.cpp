#include <cmath>
#include <cstddef>
#include <iostream>
#include <seal/modulus.h>
#include <seal/seal.h>
#include "volePSI/out/install/linux/include/cryptoTools/Common/Defines.h"
using namespace seal;
using namespace std;
int main()
{
    EncryptionParameters parms(scheme_type::bfv);

    parms.set_poly_modulus_degree(8192);
    parms.set_plain_modulus(1024);
    parms.set_coeff_modulus(CoeffModulus::BFVDefault(8192));
    parms.set_plain_modulus(PlainModulus::Batching(8192, 20));
    SEALContext context(parms);
    auto as = CoeffModulus::BFVDefault(8192);
    for (auto a : as) {
        cout << osuCrypto::log2ceil (a.value()) << " ";
    }
    cout<<'\n';
    KeyGenerator keygen(context);
    auto secret_key = keygen.secret_key();
    PublicKey public_key;
    keygen.create_public_key(public_key);
    RelinKeys relin_keys;
    keygen.create_relin_keys(relin_keys);
    GaloisKeys gal_keys;
    keygen.create_galois_keys(gal_keys);
    BatchEncoder *encoder = new BatchEncoder(context);
    Encryptor *encryptor = new Encryptor(context, public_key);
    // Evaluator *evaluator = new Evaluator(context);
    // Decryptor *decryptor = new Decryptor(context, secret_key);
    uint64_t x = 6;

    Plaintext x_plain(x);
    Ciphertext x_encrypted;
    encryptor->encrypt(x_plain, x_encrypted);
    cout << encoder->slot_count() << endl;
    cout << x_encrypted.save_size();
    // fstream file_zlib;
    // file_zlib.open("seal_zlib", ios::out );
    // x1_encrypted.save(file_zlib, compr_mode_type::zlib);
    // file_zlib.close();
    // fstream file_zstd;
    // file_zstd.open("seal_zstd", ios::out );
    // x1_encrypted.save(file_zstd, compr_mode_type::zstd);
    // file_zstd.close();
    // fstream file_none;
    // file_none.open("seal_none", ios::out );
    // x1_encrypted.save(file_none, compr_mode_type::none);
    // file_none.close();
    // cout << x1_encrypted.dyn_array().size() << endl;
}