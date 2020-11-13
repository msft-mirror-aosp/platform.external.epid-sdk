/*############################################################################
  # Copyright 2016-2019 Intel Corporation
  #
  # Licensed under the Apache License, Version 2.0 (the "License");
  # you may not use this file except in compliance with the License.
  # You may obtain a copy of the License at
  #
  #     http://www.apache.org/licenses/LICENSE-2.0
  #
  # Unless required by applicable law or agreed to in writing, software
  # distributed under the License is distributed on an "AS IS" BASIS,
  # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  # See the License for the specific language governing permissions and
  # limitations under the License.
  ############################################################################*/
/// NrProve unit tests.
/*! \file */

#include <cstring>
#include "gtest/gtest.h"
#include "testhelper/epid_gtest-testhelper.h"

extern "C" {
#include "common/sig_types.h"
#include "epid/member/split/nrprove.h"
#include "epid/member/split/signbasic.h"
#include "rlverify.h"
}

#include "member-testhelper.h"
#include "testhelper/errors-testhelper.h"
#include "testhelper/onetimepad.h"
#include "testhelper/prng-testhelper.h"
#include "testhelper/verifier_wrapper-testhelper.h"

bool operator==(SplitNrProof const& lhs, SplitNrProof const& rhs) {
  return 0 == std::memcmp(&lhs, &rhs, sizeof(lhs));
}
namespace {
#ifndef TPM_TSS  // unused function in TPM mode
void set_gid_hashalg(GroupId* id, HashAlg hashalg) {
  id->data[1] = (id->data[1] & 0xf0) | (hashalg & 0x0f);
}
#endif  // TPM_TSS
/// data for OneTimePad to be used in BasicSign: without rf and nonce
const std::vector<uint8_t> kOtpDataWithoutRfAndNonce = {
    // entropy for other operations
    // bsn in presig
    0x25, 0xeb, 0x8c, 0x48, 0xff, 0x89, 0xcb, 0x85, 0x4f, 0xc0, 0x90, 0x81,
    0xcc, 0x47, 0xed, 0xfc, 0x86, 0x19, 0xb2, 0x14, 0xfe, 0x65, 0x92, 0xd4,
    0x8b, 0xfc, 0xea, 0x9c, 0x9d, 0x8e, 0x32, 0x44,
    // r in presig =
    // 0xcf8b90f4428aaf7e9b2244d1db16848230655550f2c52b1fc53b3031f8108bd4
    // OctStr representation:
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xcf, 0x8b, 0x90, 0xf4, 0x42, 0x8a, 0xaf, 0x7e,
    0x9b, 0x22, 0x44, 0xd1, 0xdb, 0x16, 0x84, 0x82, 0x30, 0x65, 0x55, 0x50,
    0xf2, 0xc5, 0x2b, 0x1f, 0xc5, 0x3b, 0x30, 0x31, 0xf8, 0x10, 0x8b, 0xd4,
    // a = 0xfb883ef100727d4e46e3e906b96e49c68b2abe1f44084cc0ed5c5ece66afa7e9
    // OctStr representation:
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xfb, 0x88, 0x3e, 0xf1, 0x00, 0x72, 0x7d, 0x4e,
    0x46, 0xe3, 0xe9, 0x06, 0xb9, 0x6e, 0x49, 0xc6, 0x8b, 0x2a, 0xbe, 0x1f,
    0x44, 0x08, 0x4c, 0xc0, 0xed, 0x5c, 0x5e, 0xce, 0x66, 0xaf, 0xa7, 0xe8,
    // rx = 0xa1d62c80fcc1d60819271c86139880fabd27078b5dd9144c2f1dc6182fa24d4c
    // OctStr representation:
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xa1, 0xd6, 0x2c, 0x80, 0xfc, 0xc1, 0xd6, 0x08,
    0x19, 0x27, 0x1c, 0x86, 0x13, 0x98, 0x80, 0xfa, 0xbd, 0x27, 0x07, 0x8b,
    0x5d, 0xd9, 0x14, 0x4c, 0x2f, 0x1d, 0xc6, 0x18, 0x2f, 0xa2, 0x4d, 0x4b,
    // rb = 0xf6910e4edc90439572c6a46852ec550adec1b1bfd3928996605e9aa6ff97c97c
    // OctStr representation:
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xf6, 0x91, 0x0e, 0x4e, 0xdc, 0x90, 0x43, 0x95,
    0x72, 0xc6, 0xa4, 0x68, 0x52, 0xec, 0x55, 0x0a, 0xde, 0xc1, 0xb1, 0xbf,
    0xd3, 0x92, 0x89, 0x96, 0x60, 0x5e, 0x9a, 0xa6, 0xff, 0x97, 0xc9, 0x7b,
    // ra = 0x1e1e6372e53fb6a92763135b148310703e5649cfb7954d5d623aa4c1654d3147
    // OctStr representation:
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x1e, 0x1e, 0x63, 0x72, 0xe5, 0x3f, 0xb6, 0xa9,
    0x27, 0x63, 0x13, 0x5b, 0x14, 0x83, 0x10, 0x70, 0x3e, 0x56, 0x49, 0xcf,
    0xb7, 0x95, 0x4d, 0x5d, 0x62, 0x3a, 0xa4, 0xc1, 0x65, 0x4d, 0x31, 0x46};
/// data for OneTimePad to be used in BasicSign: rf in case bsn is passed
const std::vector<uint8_t> rf = {
    // rf = 0xb8b17a99305d417ee3a4fb67a60a41021a3730c05efac141d5a49b870d721d8b
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xb8, 0xb1, 0x7a, 0x99, 0x30, 0x5d, 0x41, 0x7e,
    0xe3, 0xa4, 0xfb, 0x67, 0xa6, 0x0a, 0x41, 0x02, 0x1a, 0x37, 0x30, 0xc0,
    0x5e, 0xfa, 0xc1, 0x41, 0xd5, 0xa4, 0x9b, 0x87, 0x0d, 0x72, 0x1d, 0x8b,
};
// entropy for EpidNrProve
const std::vector<uint8_t> NrProveEntropy = {
    // mu
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x9e, 0x7d, 0x36, 0x67, 0x24, 0x65, 0x9e, 0xf5,
    0x7b, 0x34, 0x2c, 0x42, 0x71, 0x4b, 0xb2, 0x58, 0xcd, 0x3d, 0x94, 0xe9,
    0x35, 0xe7, 0x37, 0x0a, 0x58, 0x32, 0xb6, 0xa5, 0x9d, 0xbf, 0xe4, 0xcb,
    // r in Tpm2Commit in PrivateExp
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x90, 0x28, 0x6d, 0x75, 0xd5, 0x44, 0x60, 0x47,
    0xe0, 0x4c, 0xf9, 0x23, 0xd2, 0x51, 0x6c, 0xca, 0xf2, 0xad, 0xd7, 0x50,
    0xd9, 0x59, 0x7d, 0x19, 0x1c, 0x53, 0x93, 0xc3, 0x75, 0x8b, 0x83, 0x6b,
    // noncek in Tpm2Sign in PrivateExp
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x82, 0x05, 0x02, 0x51, 0x5d, 0xdf, 0xa9, 0x33,
    0xce, 0x98, 0x8e, 0x95, 0xf0, 0xb5, 0x79, 0x99, 0xfe, 0xf4, 0x95, 0x33,
    0xa9, 0x23, 0x7a, 0x67, 0x60, 0xf6, 0x32, 0x5d, 0xbd, 0xfb, 0x89, 0xfa,
    // rmu
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xb4, 0x5f, 0x9c, 0x41, 0x89, 0x3c, 0xe3, 0x30,
    0x54, 0xeb, 0xa2, 0x22, 0x1b, 0x2b, 0xc4, 0xcd, 0x8f, 0x7b, 0xc8, 0xb1,
    0x1b, 0xca, 0x6f, 0x68, 0xd4, 0x9f, 0x27, 0xd9, 0xc1, 0xe5, 0xc7, 0x6d,
    // r in Tpm2Commit
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x5a, 0x1e, 0x86, 0x82, 0x29, 0x90, 0x2d, 0x57,
    0x46, 0x57, 0xa8, 0x0c, 0x73, 0xe7, 0xd4, 0xaa, 0x01, 0x11, 0x11, 0xde,
    0xd6, 0x1a, 0x58, 0x21, 0x62, 0x80, 0x2f, 0x25, 0x68, 0x23, 0xbd, 0x04,
    // noncek
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xac, 0x72, 0x37, 0x44, 0xe6, 0x90, 0x30, 0x5f,
    0x17, 0xf5, 0xf5, 0xf1, 0x9e, 0x81, 0xa9, 0x81, 0x2c, 0x7a, 0xa7, 0x37,
    0x52, 0xf6, 0x0e, 0xd3, 0xaa, 0x6c, 0x9f, 0x46, 0xf7, 0x3b, 0x2d, 0x52};
/// data for OneTimePad to be used in BasicSign: noncek of split sign
const std::vector<uint8_t> kNoncek = {
    // noncek =
    // 0x19e236b64f315e832b5ac5b68fe75d4b7c0d5f52d6cd979f76d3e7d959627f2e
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x19, 0xe2, 0x36, 0xb6, 0x4f, 0x31, 0x5e, 0x83,
    0x2b, 0x5a, 0xc5, 0xb6, 0x8f, 0xe7, 0x5d, 0x4b, 0x7c, 0x0d, 0x5f, 0x52,
    0xd6, 0xcd, 0x97, 0x9f, 0x76, 0xd3, 0xe7, 0xd9, 0x59, 0x62, 0x7f, 0x2e,
};
TEST_F(EpidSplitMemberTest, NrProveFailsGivenNullParameters) {
  Prng my_prng;
  PrivKey mpriv_key = this->kGrpXMember3PrivKeySha256;
  GroupPubKey pub_key = this->kGrpXKey;
  MemberCtxObj member(pub_key, mpriv_key, this->kMemberPrecomp, &Prng::Generate,
                      &my_prng);

  BasicSignature const* basic_sig =
      &reinterpret_cast<EpidNonSplitSignature const*>(
           this->kSplitSigGrpXMember3Sha256Basename1Test1WithSigRl.data())
           ->sigma0;
  auto& msg = this->kTest1Msg;
  auto& bsn = this->kBsn0;
  SigRl const* sig_rl = reinterpret_cast<const SigRl*>(this->kSigRlData.data());

  SplitNrProof proof;

  EXPECT_EQ(kEpidBadArgErr,
            EpidNrProve(nullptr, msg.data(), msg.size(), bsn.data(), bsn.size(),
                        basic_sig, &sig_rl->bk[0], &proof));
  EXPECT_EQ(kEpidBadArgErr,
            EpidNrProve(member, nullptr, msg.size(), bsn.data(), bsn.size(),
                        basic_sig, &sig_rl->bk[0], &proof));
  EXPECT_EQ(kEpidBadArgErr, EpidNrProve(member, msg.data(), msg.size(), nullptr,
                                        0, basic_sig, &sig_rl->bk[0], &proof));
  EXPECT_EQ(kEpidBadArgErr,
            EpidNrProve(member, msg.data(), msg.size(), bsn.data(), 0,
                        basic_sig, &sig_rl->bk[0], &proof));
  EXPECT_EQ(kEpidBadArgErr,
            EpidNrProve(member, msg.data(), msg.size(), bsn.data(), bsn.size(),
                        nullptr, &sig_rl->bk[0], &proof));
  EXPECT_EQ(kEpidBadArgErr,
            EpidNrProve(member, msg.data(), msg.size(), bsn.data(), bsn.size(),
                        basic_sig, nullptr, &proof));
  EXPECT_EQ(kEpidBadArgErr,
            EpidNrProve(member, msg.data(), msg.size(), bsn.data(), bsn.size(),
                        basic_sig, &sig_rl->bk[0], nullptr));
}

TEST_F(EpidSplitMemberTest, NrProveFailsGivenInvalidSigRlEntry) {
  Prng my_prng;
  PrivKey mpriv_key = this->kGrpXMember3PrivKeySha256;
  GroupPubKey pub_key = this->kGrpXKey;
  MemberCtxObj member(pub_key, mpriv_key, this->kMemberPrecomp, &Prng::Generate,
                      &my_prng);

  BasicSignature basic_sig;
  auto msg = this->kTest1Msg;
  SigRl const* sig_rl = reinterpret_cast<const SigRl*>(this->kSigRlData.data());
  SplitNrProof proof;
  BigNumStr rnd_bsn = {0};
  FpElemStr nonce_k;

  THROW_ON_EPIDERR(EpidSplitSignBasic(member, msg.data(), msg.size(), nullptr,
                                      0, &basic_sig, &rnd_bsn, &nonce_k));

  SigRlEntry sig_rl_entry_invalid_k = sig_rl->bk[0];
  sig_rl_entry_invalid_k.k.x.data.data[31]++;  // make it not in EC group
  EXPECT_EQ(kEpidBadArgErr, EpidNrProve(member, msg.data(), msg.size(),
                                        &rnd_bsn, sizeof(rnd_bsn), &basic_sig,
                                        &sig_rl_entry_invalid_k, &proof));

  SigRlEntry sig_rl_entry_invalid_b = sig_rl->bk[0];
  sig_rl_entry_invalid_b.b.x.data.data[31]++;  // make it not in EC group
  EXPECT_EQ(kEpidBadArgErr, EpidNrProve(member, msg.data(), msg.size(),
                                        &rnd_bsn, sizeof(rnd_bsn), &basic_sig,
                                        &sig_rl_entry_invalid_b, &proof));
}

TEST_F(EpidSplitMemberTest,
       PROTECTED_NrProveFailsWithInvalidSigRlEntryAndCredential_EPS0) {
  Prng my_prng;
  PrivKey mpriv_key = this->kEps0MemberPrivateKey;
  GroupPubKey pub_key = this->kEps0GroupPublicKey;
  MemberCtxObj member(pub_key, *(MembershipCredential const*)&mpriv_key,
                      &Prng::Generate, &my_prng);

  BasicSignature basic_sig;
  auto msg = this->kTest1Msg;
  SigRl const* sig_rl = reinterpret_cast<const SigRl*>(this->kSigRlData.data());
  SplitNrProof proof;
  BigNumStr rnd_bsn = {0};
  FpElemStr nonce_k;

  THROW_ON_EPIDERR(EpidSplitSignBasic(member, msg.data(), msg.size(), nullptr,
                                      0, &basic_sig, &rnd_bsn, &nonce_k));

  SigRlEntry sig_rl_entry_invalid_k = sig_rl->bk[0];
  sig_rl_entry_invalid_k.k.x.data.data[31]++;  // make it not in EC group
  EXPECT_EQ(kEpidBadArgErr, EpidNrProve(member, msg.data(), msg.size(),
                                        &rnd_bsn, sizeof(rnd_bsn), &basic_sig,
                                        &sig_rl_entry_invalid_k, &proof));

  SigRlEntry sig_rl_entry_invalid_b = sig_rl->bk[0];
  sig_rl_entry_invalid_b.b.x.data.data[31]++;  // make it not in EC group
  EXPECT_EQ(kEpidBadArgErr, EpidNrProve(member, msg.data(), msg.size(),
                                        &rnd_bsn, sizeof(rnd_bsn), &basic_sig,
                                        &sig_rl_entry_invalid_b, &proof));
}

TEST_F(EpidSplitMemberTest, NrProveFailsGivenInvalidBasicSig) {
  Prng my_prng;
  PrivKey mpriv_key = this->kGrpXMember3PrivKeySha256;
  GroupPubKey pub_key = this->kGrpXKey;
  MemberCtxObj member(pub_key, mpriv_key, this->kMemberPrecomp, &Prng::Generate,
                      &my_prng);

  BasicSignature basic_sig;
  auto msg = this->kTest1Msg;
  SigRl const* sig_rl = reinterpret_cast<const SigRl*>(this->kSigRlData.data());
  SplitNrProof proof;
  BigNumStr rnd_bsn = {0};
  FpElemStr nonce_k;

  THROW_ON_EPIDERR(EpidSplitSignBasic(member, msg.data(), msg.size(), nullptr,
                                      0, &basic_sig, &rnd_bsn, &nonce_k));

  // invalid basic sig is only when K value is invalid!!
  BasicSignature basic_sig_invalid_K = basic_sig;
  basic_sig_invalid_K.K.x.data.data[31]++;  // make it not in EC group
  EXPECT_EQ(
      kEpidBadArgErr,
      EpidNrProve(member, msg.data(), msg.size(), &rnd_bsn, sizeof(rnd_bsn),
                  &basic_sig_invalid_K, &sig_rl->bk[0], &proof));
}

TEST_F(EpidSplitMemberTest,
       PROTECTED_NrProveFailsGivenInvalidBasicSigAndCredential_EPS0) {
  Prng my_prng;
  PrivKey mpriv_key = this->kEps0MemberPrivateKey;
  GroupPubKey pub_key = this->kEps0GroupPublicKey;
  MemberCtxObj member(pub_key, *(MembershipCredential const*)&mpriv_key,
                      &Prng::Generate, &my_prng);

  BasicSignature basic_sig;
  auto msg = this->kTest1Msg;
  SigRl const* sig_rl = reinterpret_cast<const SigRl*>(this->kSigRlData.data());
  SplitNrProof proof;
  BigNumStr rnd_bsn = {0};
  FpElemStr nonce_k;

  THROW_ON_EPIDERR(EpidSplitSignBasic(member, msg.data(), msg.size(), nullptr,
                                      0, &basic_sig, &rnd_bsn, &nonce_k));

  // invalid basic sig is only when K value is invalid!!
  BasicSignature basic_sig_invalid_K = basic_sig;
  basic_sig_invalid_K.K.x.data.data[31]++;  // make it not in EC group
  EXPECT_EQ(
      kEpidBadArgErr,
      EpidNrProve(member, msg.data(), msg.size(), &rnd_bsn, sizeof(rnd_bsn),
                  &basic_sig_invalid_K, &sig_rl->bk[0], &proof));
}

// NOTE: Do not run these tests in TPM HW mode because some of the data is
// generated randomly inside TPM and will not match with precomputed data
#ifndef TPM_TSS
TEST_F(EpidSplitMemberTest, GeneratesNrProofForEmptyMessage) {
  PrivKey mpriv_key = this->kGrpXMember3PrivKeySha256;
  GroupPubKey pub_key = this->kGrpXKey;
  auto otp_data = kOtpDataWithoutRfAndNonce;
  otp_data.insert(otp_data.end(), kNoncek.begin(), kNoncek.end());
  otp_data.insert(otp_data.end(), NrProveEntropy.begin(), NrProveEntropy.end());
  OneTimePad my_prng(otp_data);
  MemberCtxObj member(pub_key, mpriv_key, this->kMemberPrecomp,
                      &OneTimePad::Generate, &my_prng);

  BasicSignature basic_sig;
  SigRl const* sig_rl = reinterpret_cast<const SigRl*>(this->kSigRlData.data());
  BigNumStr rnd_bsn = {0};

  SplitNrProof proof;
  FpElemStr nonce_k;

  ASSERT_EQ(kEpidNoErr, EpidSplitSignBasic(member, nullptr, 0, nullptr, 0,
                                           &basic_sig, &rnd_bsn, &nonce_k));
  EXPECT_EQ(kEpidNoErr,
            EpidNrProve(member, nullptr, 0, &rnd_bsn, sizeof(rnd_bsn),
                        &basic_sig, &sig_rl->bk[0], &proof));
  SplitNrProof expected{
      // T
      0x63, 0x67, 0x0e, 0x23, 0x5c, 0xaa, 0xd0, 0x22, 0xc8, 0x1e, 0x33, 0xf2,
      0x31, 0x54, 0x28, 0xa7, 0xeb, 0x8d, 0xef, 0xb0, 0xab, 0x9e, 0x97, 0x6c,
      0xf2, 0xdc, 0x18, 0xc2, 0x4c, 0xec, 0x2f, 0x3d, 0xa4, 0x84, 0x58, 0xa5,
      0x1e, 0xed, 0x25, 0x5d, 0x0d, 0x10, 0x80, 0x3e, 0x08, 0x65, 0x36, 0x61,
      0xc0, 0xd9, 0xc1, 0xce, 0x70, 0xe8, 0x10, 0xfd, 0x14, 0xd3, 0xfd, 0xf1,
      0x47, 0x8f, 0x77, 0x65,
      // c
      0xf9, 0x8d, 0x7c, 0x71, 0xed, 0x8d, 0x6e, 0xda, 0x1a, 0x7b, 0xf6, 0xe4,
      0xd8, 0xe4, 0x80, 0x74, 0xa6, 0x7a, 0x7e, 0x2d, 0xbe, 0x4c, 0x58, 0xa9,
      0xff, 0xa6, 0xbc, 0x23, 0x0f, 0x5b, 0x0b, 0x48,
      // smu
      0xd1, 0xdb, 0xa6, 0xa5, 0x23, 0x9b, 0xec, 0xcd, 0x7a, 0x94, 0x29, 0x3f,
      0x5c, 0x54, 0x10, 0xdc, 0xec, 0xaa, 0x13, 0xb3, 0x7c, 0x78, 0x5c, 0x8c,
      0xd2, 0xb1, 0xbc, 0xfa, 0x69, 0x0f, 0xdd, 0x43,
      // snu
      0xf4, 0x91, 0xb5, 0x4d, 0x14, 0x3c, 0xe4, 0x4f, 0x8d, 0x05, 0x9b, 0xa3,
      0x5c, 0x49, 0x0b, 0xd9, 0x2a, 0xc7, 0x8a, 0x2a, 0x65, 0x54, 0x7a, 0x50,
      0x3e, 0x52, 0xd7, 0xa4, 0xde, 0x15, 0x24, 0x15,
      // k
      0xac, 0x72, 0x37, 0x44, 0xe6, 0x90, 0x30, 0x5f, 0x17, 0xf5, 0xf5, 0xf1,
      0x9e, 0x81, 0xa9, 0x81, 0x2c, 0x7a, 0xa7, 0x37, 0x52, 0xf6, 0x0e, 0xd3,
      0xaa, 0x6c, 0x9f, 0x46, 0xf7, 0x3b, 0x2d, 0x52};
  EXPECT_EQ(expected, proof);
}

TEST_F(EpidSplitMemberTest, GeneratesNrProofForMsgContainingAllPossibleBytes) {
  PrivKey mpriv_key = this->kGrpXMember3PrivKeySha256;
  GroupPubKey pub_key = this->kGrpXKey;
  auto otp_data = kOtpDataWithoutRfAndNonce;
  otp_data.insert(otp_data.end(), kNoncek.begin(), kNoncek.end());
  otp_data.insert(otp_data.end(), NrProveEntropy.begin(), NrProveEntropy.end());
  OneTimePad my_prng(otp_data);
  MemberCtxObj member(pub_key, mpriv_key, this->kMemberPrecomp,
                      &OneTimePad::Generate, &my_prng);

  BasicSignature basic_sig;
  auto msg = this->kData_0_255;
  BigNumStr rnd_bsn = {0};
  SigRl const* sig_rl = reinterpret_cast<const SigRl*>(this->kSigRlData.data());

  SplitNrProof proof;
  FpElemStr nonce_k;

  ASSERT_EQ(kEpidNoErr,
            EpidSplitSignBasic(member, msg.data(), msg.size(), nullptr, 0,
                               &basic_sig, &rnd_bsn, &nonce_k));
  EXPECT_EQ(kEpidNoErr,
            EpidNrProve(member, msg.data(), msg.size(), &rnd_bsn,
                        sizeof(rnd_bsn), &basic_sig, &sig_rl->bk[0], &proof));

  SplitNrProof expected{
      // T
      0x63, 0x67, 0x0e, 0x23, 0x5c, 0xaa, 0xd0, 0x22, 0xc8, 0x1e, 0x33, 0xf2,
      0x31, 0x54, 0x28, 0xa7, 0xeb, 0x8d, 0xef, 0xb0, 0xab, 0x9e, 0x97, 0x6c,
      0xf2, 0xdc, 0x18, 0xc2, 0x4c, 0xec, 0x2f, 0x3d, 0xa4, 0x84, 0x58, 0xa5,
      0x1e, 0xed, 0x25, 0x5d, 0x0d, 0x10, 0x80, 0x3e, 0x08, 0x65, 0x36, 0x61,
      0xc0, 0xd9, 0xc1, 0xce, 0x70, 0xe8, 0x10, 0xfd, 0x14, 0xd3, 0xfd, 0xf1,
      0x47, 0x8f, 0x77, 0x65,
      // c
      0x93, 0x32, 0x0a, 0xf3, 0xf5, 0xee, 0x62, 0x90, 0x11, 0x75, 0x83, 0xe4,
      0xf6, 0xe6, 0xb6, 0x7b, 0x04, 0x46, 0xa3, 0xcd, 0xeb, 0x37, 0x3c, 0x86,
      0xf4, 0xee, 0x0f, 0x00, 0x8b, 0x1b, 0x63, 0xde,
      // smu
      0xf7, 0x09, 0x26, 0xb0, 0x58, 0x0f, 0xfd, 0x45, 0xa8, 0x50, 0x77, 0x38,
      0xfa, 0x55, 0x3b, 0x50, 0x36, 0x51, 0xe6, 0x07, 0x12, 0xe8, 0xcc, 0x6f,
      0x97, 0x53, 0xc8, 0xd9, 0xef, 0xde, 0x3a, 0x85,
      // snu
      0x34, 0x58, 0x9c, 0x67, 0x3b, 0xa2, 0x4b, 0x77, 0x76, 0xed, 0xb6, 0x1d,
      0xd3, 0x04, 0x73, 0xbc, 0x58, 0xd4, 0xb6, 0xa8, 0x00, 0xa5, 0x28, 0x6a,
      0xf6, 0x36, 0x5b, 0xd3, 0x43, 0x47, 0x04, 0xd7,
      // k
      0xac, 0x72, 0x37, 0x44, 0xe6, 0x90, 0x30, 0x5f, 0x17, 0xf5, 0xf5, 0xf1,
      0x9e, 0x81, 0xa9, 0x81, 0x2c, 0x7a, 0xa7, 0x37, 0x52, 0xf6, 0x0e, 0xd3,
      0xaa, 0x6c, 0x9f, 0x46, 0xf7, 0x3b, 0x2d, 0x52};
  EXPECT_EQ(expected, proof);
}

TEST_F(EpidSplitMemberTest, GeneratesNrProof) {
  PrivKey mpriv_key = this->kGrpXMember3PrivKeySha256;
  GroupPubKey pub_key = this->kGrpXKey;
  auto otp_data = kOtpDataWithoutRfAndNonce;
  otp_data.insert(otp_data.end(), kNoncek.begin(), kNoncek.end());
  otp_data.insert(otp_data.end(), NrProveEntropy.begin(), NrProveEntropy.end());
  OneTimePad my_prng(otp_data);
  MemberCtxObj member(pub_key, mpriv_key, this->kMemberPrecomp,
                      &OneTimePad::Generate, &my_prng);

  BasicSignature basic_sig;
  auto msg = this->kTest1Msg;
  SigRl const* sig_rl = reinterpret_cast<const SigRl*>(this->kSigRlData.data());
  BigNumStr rnd_bsn = {0};

  SplitNrProof proof;
  FpElemStr nonce_k;

  ASSERT_EQ(kEpidNoErr,
            EpidSplitSignBasic(member, msg.data(), msg.size(), nullptr, 0,
                               &basic_sig, &rnd_bsn, &nonce_k));
  EXPECT_EQ(kEpidNoErr,
            EpidNrProve(member, msg.data(), msg.size(), &rnd_bsn,
                        sizeof(rnd_bsn), &basic_sig, &sig_rl->bk[0], &proof));

  SplitNrProof expected{
      // T
      0x63, 0x67, 0x0e, 0x23, 0x5c, 0xaa, 0xd0, 0x22, 0xc8, 0x1e, 0x33, 0xf2,
      0x31, 0x54, 0x28, 0xa7, 0xeb, 0x8d, 0xef, 0xb0, 0xab, 0x9e, 0x97, 0x6c,
      0xf2, 0xdc, 0x18, 0xc2, 0x4c, 0xec, 0x2f, 0x3d, 0xa4, 0x84, 0x58, 0xa5,
      0x1e, 0xed, 0x25, 0x5d, 0x0d, 0x10, 0x80, 0x3e, 0x08, 0x65, 0x36, 0x61,
      0xc0, 0xd9, 0xc1, 0xce, 0x70, 0xe8, 0x10, 0xfd, 0x14, 0xd3, 0xfd, 0xf1,
      0x47, 0x8f, 0x77, 0x65,
      // c
      0x4e, 0x89, 0xc2, 0x04, 0xde, 0x4c, 0x9c, 0xc2, 0x61, 0xf9, 0x47, 0xe7,
      0x30, 0x7e, 0xdd, 0x5c, 0x02, 0xdf, 0xc9, 0x8f, 0x02, 0xcf, 0x27, 0xd8,
      0xe3, 0x5a, 0xd1, 0x21, 0xe8, 0x58, 0xca, 0xaf,
      // smu
      0xd6, 0x0c, 0x92, 0x4e, 0xfb, 0x31, 0xb7, 0x7f, 0x0f, 0x25, 0x0a, 0x1c,
      0x65, 0xe5, 0x44, 0xe1, 0x78, 0x82, 0x2f, 0x40, 0x7f, 0xfb, 0xac, 0x09,
      0x09, 0xf2, 0xd6, 0x39, 0x12, 0x0a, 0x69, 0x1a,
      // snu
      0x46, 0xd6, 0xc0, 0x3e, 0x74, 0x88, 0x38, 0xf3, 0x8b, 0x58, 0xc5, 0xd5,
      0x31, 0xce, 0xcd, 0x3b, 0x9b, 0xc0, 0x0c, 0xe5, 0x60, 0x35, 0x4a, 0xc7,
      0xcc, 0x12, 0x36, 0x4c, 0xf8, 0x0d, 0x4f, 0xe8,
      // k
      0xac, 0x72, 0x37, 0x44, 0xe6, 0x90, 0x30, 0x5f, 0x17, 0xf5, 0xf5, 0xf1,
      0x9e, 0x81, 0xa9, 0x81, 0x2c, 0x7a, 0xa7, 0x37, 0x52, 0xf6, 0x0e, 0xd3,
      0xaa, 0x6c, 0x9f, 0x46, 0xf7, 0x3b, 0x2d, 0x52};
  EXPECT_EQ(expected, proof);
}

TEST_F(EpidSplitMemberTest, GeneratesNrProofUsingSha256HashAlg) {
  PrivKey mpriv_key = this->kGrpXMember3PrivKeySha256;
  GroupPubKey pub_key = this->kGrpXKey;
  set_gid_hashalg(&mpriv_key.gid, kSha256);
  set_gid_hashalg(&pub_key.gid, kSha256);
  auto otp_data = kOtpDataWithoutRfAndNonce;
  otp_data.insert(otp_data.end(), kNoncek.begin(), kNoncek.end());
  otp_data.insert(otp_data.end(), NrProveEntropy.begin(), NrProveEntropy.end());
  OneTimePad my_prng(otp_data);
  MemberCtxObj member(pub_key, mpriv_key, this->kMemberPrecomp,
                      &OneTimePad::Generate, &my_prng);

  BasicSignature basic_sig;
  auto msg = this->kTest1Msg;
  SigRl const* sig_rl = reinterpret_cast<const SigRl*>(this->kSigRlData.data());
  BigNumStr rnd_bsn = {0};

  SplitNrProof proof;
  FpElemStr nonce_k;

  ASSERT_EQ(kEpidNoErr,
            EpidSplitSignBasic(member, msg.data(), msg.size(), nullptr, 0,
                               &basic_sig, &rnd_bsn, &nonce_k));
  EXPECT_EQ(kEpidNoErr,
            EpidNrProve(member, msg.data(), msg.size(), &rnd_bsn,
                        sizeof(rnd_bsn), &basic_sig, &sig_rl->bk[0], &proof));

  SplitNrProof expected{
      // T
      0x63, 0x67, 0x0e, 0x23, 0x5c, 0xaa, 0xd0, 0x22, 0xc8, 0x1e, 0x33, 0xf2,
      0x31, 0x54, 0x28, 0xa7, 0xeb, 0x8d, 0xef, 0xb0, 0xab, 0x9e, 0x97, 0x6c,
      0xf2, 0xdc, 0x18, 0xc2, 0x4c, 0xec, 0x2f, 0x3d, 0xa4, 0x84, 0x58, 0xa5,
      0x1e, 0xed, 0x25, 0x5d, 0x0d, 0x10, 0x80, 0x3e, 0x08, 0x65, 0x36, 0x61,
      0xc0, 0xd9, 0xc1, 0xce, 0x70, 0xe8, 0x10, 0xfd, 0x14, 0xd3, 0xfd, 0xf1,
      0x47, 0x8f, 0x77, 0x65,
      // c
      0x4e, 0x89, 0xc2, 0x04, 0xde, 0x4c, 0x9c, 0xc2, 0x61, 0xf9, 0x47, 0xe7,
      0x30, 0x7e, 0xdd, 0x5c, 0x02, 0xdf, 0xc9, 0x8f, 0x02, 0xcf, 0x27, 0xd8,
      0xe3, 0x5a, 0xd1, 0x21, 0xe8, 0x58, 0xca, 0xaf,
      // smu
      0xd6, 0x0c, 0x92, 0x4e, 0xfb, 0x31, 0xb7, 0x7f, 0x0f, 0x25, 0x0a, 0x1c,
      0x65, 0xe5, 0x44, 0xe1, 0x78, 0x82, 0x2f, 0x40, 0x7f, 0xfb, 0xac, 0x09,
      0x09, 0xf2, 0xd6, 0x39, 0x12, 0x0a, 0x69, 0x1a,
      // snu
      0x46, 0xd6, 0xc0, 0x3e, 0x74, 0x88, 0x38, 0xf3, 0x8b, 0x58, 0xc5, 0xd5,
      0x31, 0xce, 0xcd, 0x3b, 0x9b, 0xc0, 0x0c, 0xe5, 0x60, 0x35, 0x4a, 0xc7,
      0xcc, 0x12, 0x36, 0x4c, 0xf8, 0x0d, 0x4f, 0xe8,
      // k
      0xac, 0x72, 0x37, 0x44, 0xe6, 0x90, 0x30, 0x5f, 0x17, 0xf5, 0xf5, 0xf1,
      0x9e, 0x81, 0xa9, 0x81, 0x2c, 0x7a, 0xa7, 0x37, 0x52, 0xf6, 0x0e, 0xd3,
      0xaa, 0x6c, 0x9f, 0x46, 0xf7, 0x3b, 0x2d, 0x52};
  EXPECT_EQ(expected, proof);
}

TEST_F(EpidSplitMemberTest, GeneratesNrProofUsingSha384HashAlg) {
  PrivKey mpriv_key = this->kGrpXMember3PrivKeySha384;
  GroupPubKey pub_key = this->kGrpXKey;
  set_gid_hashalg(&mpriv_key.gid, kSha384);
  set_gid_hashalg(&pub_key.gid, kSha384);
  auto otp_data = kOtpDataWithoutRfAndNonce;
  otp_data.insert(otp_data.end(), kNoncek.begin(), kNoncek.end());
  otp_data.insert(otp_data.end(), NrProveEntropy.begin(), NrProveEntropy.end());
  OneTimePad my_prng(otp_data);
  MemberCtxObj member(pub_key, mpriv_key, this->kMemberPrecomp,
                      &OneTimePad::Generate, &my_prng);

  BasicSignature basic_sig;
  auto msg = this->kTest1Msg;
  SigRl const* sig_rl = reinterpret_cast<const SigRl*>(this->kSigRlData.data());
  BigNumStr rnd_bsn = {0};

  SplitNrProof proof;
  FpElemStr nonce_k;

  ASSERT_EQ(kEpidNoErr,
            EpidSplitSignBasic(member, msg.data(), msg.size(), nullptr, 0,
                               &basic_sig, &rnd_bsn, &nonce_k));
  EXPECT_EQ(kEpidNoErr,
            EpidNrProve(member, msg.data(), msg.size(), &rnd_bsn,
                        sizeof(rnd_bsn), &basic_sig, &sig_rl->bk[0], &proof));

  SplitNrProof expected{
      // T
      0x63, 0x67, 0x0e, 0x23, 0x5c, 0xaa, 0xd0, 0x22, 0xc8, 0x1e, 0x33, 0xf2,
      0x31, 0x54, 0x28, 0xa7, 0xeb, 0x8d, 0xef, 0xb0, 0xab, 0x9e, 0x97, 0x6c,
      0xf2, 0xdc, 0x18, 0xc2, 0x4c, 0xec, 0x2f, 0x3d, 0xa4, 0x84, 0x58, 0xa5,
      0x1e, 0xed, 0x25, 0x5d, 0x0d, 0x10, 0x80, 0x3e, 0x08, 0x65, 0x36, 0x61,
      0xc0, 0xd9, 0xc1, 0xce, 0x70, 0xe8, 0x10, 0xfd, 0x14, 0xd3, 0xfd, 0xf1,
      0x47, 0x8f, 0x77, 0x65,
      // c
      0x1f, 0x2d, 0x94, 0x59, 0xb3, 0x0a, 0x81, 0x0c, 0x5c, 0x2c, 0x57, 0x66,
      0x5d, 0x42, 0xe5, 0xb3, 0x63, 0x01, 0xb9, 0x67, 0xac, 0x37, 0xba, 0xdf,
      0x85, 0x63, 0x01, 0x24, 0x73, 0xc8, 0x25, 0x84,
      // smu
      0x94, 0x10, 0xe9, 0x07, 0x13, 0x58, 0x0c, 0x12, 0x59, 0x42, 0x8b, 0xd6,
      0x90, 0x10, 0x77, 0xfb, 0x50, 0x26, 0x69, 0xc4, 0x5e, 0x9c, 0x62, 0xdc,
      0xed, 0x8f, 0xb9, 0xa4, 0xbc, 0x98, 0x24, 0xb4,
      // snu
      0x73, 0xa6, 0x44, 0x1b, 0x78, 0x8a, 0x04, 0xfb, 0x7b, 0x43, 0xf4, 0x1a,
      0x37, 0xea, 0x30, 0x68, 0x3c, 0x1d, 0x38, 0x77, 0x2c, 0xf6, 0x95, 0x7f,
      0xab, 0x38, 0x60, 0x2c, 0x22, 0xd8, 0x81, 0xb3,
      // k
      0xac, 0x72, 0x37, 0x44, 0xe6, 0x90, 0x30, 0x5f, 0x17, 0xf5, 0xf5, 0xf1,
      0x9e, 0x81, 0xa9, 0x81, 0x2c, 0x7a, 0xa7, 0x37, 0x52, 0xf6, 0x0e, 0xd3,
      0xaa, 0x6c, 0x9f, 0x46, 0xf7, 0x3b, 0x2d, 0x52};
  EXPECT_EQ(expected, proof);
}

TEST_F(EpidSplitMemberTest, GeneratesNrProofUsingSha512HashAlg) {
  PrivKey mpriv_key = this->kGrpXMember3PrivKeySha512;
  GroupPubKey pub_key = this->kGrpXKey;
  set_gid_hashalg(&mpriv_key.gid, kSha512);
  set_gid_hashalg(&pub_key.gid, kSha512);
  auto otp_data = kOtpDataWithoutRfAndNonce;
  otp_data.insert(otp_data.end(), kNoncek.begin(), kNoncek.end());
  otp_data.insert(otp_data.end(), NrProveEntropy.begin(), NrProveEntropy.end());
  OneTimePad my_prng(otp_data);
  MemberCtxObj member(pub_key, mpriv_key, this->kMemberPrecomp,
                      &OneTimePad::Generate, &my_prng);

  BasicSignature basic_sig;
  auto msg = this->kTest1Msg;
  SigRl const* sig_rl = reinterpret_cast<const SigRl*>(this->kSigRlData.data());
  BigNumStr rnd_bsn = {0};

  SplitNrProof proof;
  FpElemStr nonce_k;

  ASSERT_EQ(kEpidNoErr,
            EpidSplitSignBasic(member, msg.data(), msg.size(), nullptr, 0,
                               &basic_sig, &rnd_bsn, &nonce_k));
  EXPECT_EQ(kEpidNoErr,
            EpidNrProve(member, msg.data(), msg.size(), &rnd_bsn,
                        sizeof(rnd_bsn), &basic_sig, &sig_rl->bk[0], &proof));

  SplitNrProof expected{
      // T
      0x63, 0x67, 0x0e, 0x23, 0x5c, 0xaa, 0xd0, 0x22, 0xc8, 0x1e, 0x33, 0xf2,
      0x31, 0x54, 0x28, 0xa7, 0xeb, 0x8d, 0xef, 0xb0, 0xab, 0x9e, 0x97, 0x6c,
      0xf2, 0xdc, 0x18, 0xc2, 0x4c, 0xec, 0x2f, 0x3d, 0xa4, 0x84, 0x58, 0xa5,
      0x1e, 0xed, 0x25, 0x5d, 0x0d, 0x10, 0x80, 0x3e, 0x08, 0x65, 0x36, 0x61,
      0xc0, 0xd9, 0xc1, 0xce, 0x70, 0xe8, 0x10, 0xfd, 0x14, 0xd3, 0xfd, 0xf1,
      0x47, 0x8f, 0x77, 0x65,
      // c
      0xc9, 0xd6, 0x59, 0xce, 0xa3, 0x65, 0x0a, 0x0c, 0x07, 0x77, 0xd6, 0x76,
      0x1a, 0x4e, 0xe0, 0x6f, 0x06, 0x4a, 0xaa, 0x32, 0x73, 0xc1, 0x0a, 0x19,
      0xc3, 0xba, 0xd8, 0xea, 0x8c, 0xbc, 0x5d, 0xc5,
      // smu
      0xa7, 0xfb, 0x2d, 0x71, 0x5d, 0xf6, 0xe9, 0x97, 0x15, 0xd4, 0xe4, 0xd8,
      0x10, 0x73, 0x4b, 0x31, 0x06, 0x1a, 0x7e, 0x0c, 0x1e, 0xa2, 0x5e, 0x36,
      0xdd, 0x79, 0x70, 0xc2, 0xfc, 0x37, 0x83, 0x63,
      // snu
      0x8b, 0x83, 0x02, 0x03, 0x1d, 0x0b, 0xf1, 0x36, 0x0b, 0x04, 0x5f, 0x32,
      0xf7, 0x7b, 0x53, 0x0d, 0xb5, 0x90, 0xf6, 0xf2, 0x34, 0xb2, 0x43, 0xa8,
      0x08, 0x77, 0xfa, 0x8b, 0x8c, 0xe1, 0xc1, 0x48,
      // k
      0xac, 0x72, 0x37, 0x44, 0xe6, 0x90, 0x30, 0x5f, 0x17, 0xf5, 0xf5, 0xf1,
      0x9e, 0x81, 0xa9, 0x81, 0x2c, 0x7a, 0xa7, 0x37, 0x52, 0xf6, 0x0e, 0xd3,
      0xaa, 0x6c, 0x9f, 0x46, 0xf7, 0x3b, 0x2d, 0x52};
  EXPECT_EQ(expected, proof);
}

TEST_F(EpidSplitMemberTest, GeneratesNrProofUsingSha512256HashAlg) {
  PrivKey mpriv_key = this->kGrpXMember3PrivKeySha512256;
  GroupPubKey pub_key = this->kGrpXKey;
  set_gid_hashalg(&mpriv_key.gid, kSha512_256);
  set_gid_hashalg(&pub_key.gid, kSha512_256);
  auto otp_data = kOtpDataWithoutRfAndNonce;
  otp_data.insert(otp_data.end(), kNoncek.begin(), kNoncek.end());
  otp_data.insert(otp_data.end(), NrProveEntropy.begin(), NrProveEntropy.end());
  OneTimePad my_prng(otp_data);
  MemberCtxObj member(pub_key, mpriv_key, this->kMemberPrecomp,
                      &OneTimePad::Generate, &my_prng);

  BasicSignature basic_sig;
  auto msg = this->kTest1Msg;
  SigRl const* sig_rl = reinterpret_cast<const SigRl*>(this->kSigRlData.data());
  BigNumStr rnd_bsn = {0};

  SplitNrProof proof;
  FpElemStr nonce_k;

  ASSERT_EQ(kEpidNoErr,
            EpidSplitSignBasic(member, msg.data(), msg.size(), nullptr, 0,
                               &basic_sig, &rnd_bsn, &nonce_k));
  EXPECT_EQ(kEpidNoErr,
            EpidNrProve(member, msg.data(), msg.size(), &rnd_bsn,
                        sizeof(rnd_bsn), &basic_sig, &sig_rl->bk[0], &proof));

  SplitNrProof expected{
      // T
      0x63, 0x67, 0x0e, 0x23, 0x5c, 0xaa, 0xd0, 0x22, 0xc8, 0x1e, 0x33, 0xf2,
      0x31, 0x54, 0x28, 0xa7, 0xeb, 0x8d, 0xef, 0xb0, 0xab, 0x9e, 0x97, 0x6c,
      0xf2, 0xdc, 0x18, 0xc2, 0x4c, 0xec, 0x2f, 0x3d, 0xa4, 0x84, 0x58, 0xa5,
      0x1e, 0xed, 0x25, 0x5d, 0x0d, 0x10, 0x80, 0x3e, 0x08, 0x65, 0x36, 0x61,
      0xc0, 0xd9, 0xc1, 0xce, 0x70, 0xe8, 0x10, 0xfd, 0x14, 0xd3, 0xfd, 0xf1,
      0x47, 0x8f, 0x77, 0x65,
      // c
      0x37, 0x3f, 0xea, 0x79, 0x40, 0x87, 0x65, 0xce, 0xcf, 0xcf, 0xdf, 0x6c,
      0xd3, 0x2a, 0x6c, 0xcb, 0xdb, 0xef, 0xec, 0xf8, 0xf9, 0x1c, 0x14, 0x04,
      0xd2, 0x1a, 0xd1, 0xab, 0x6b, 0x85, 0x06, 0x1e,
      // smu
      0x0b, 0xaa, 0x76, 0xf1, 0x0a, 0x1e, 0x54, 0x64, 0xdf, 0x2f, 0x2a, 0x3d,
      0x4f, 0xfd, 0xc1, 0xc7, 0x9b, 0xd7, 0xd4, 0x23, 0xff, 0xbe, 0x99, 0x03,
      0xa0, 0x56, 0x2d, 0x42, 0x25, 0x88, 0x2d, 0x0f,
      // snu
      0x39, 0xb2, 0x07, 0x80, 0x52, 0xbb, 0x7d, 0x48, 0xd7, 0x24, 0x2e, 0x0e,
      0xac, 0x1a, 0x25, 0xfd, 0xef, 0x18, 0xc6, 0xc2, 0x34, 0x59, 0xc5, 0x87,
      0x5f, 0xe9, 0x91, 0xf9, 0x9a, 0xad, 0x9c, 0x22,
      // k
      0xac, 0x72, 0x37, 0x44, 0xe6, 0x90, 0x30, 0x5f, 0x17, 0xf5, 0xf5, 0xf1,
      0x9e, 0x81, 0xa9, 0x81, 0x2c, 0x7a, 0xa7, 0x37, 0x52, 0xf6, 0x0e, 0xd3,
      0xaa, 0x6c, 0x9f, 0x46, 0xf7, 0x3b, 0x2d, 0x52};
  EXPECT_EQ(expected, proof);
}
#endif

TEST_F(EpidSplitMemberTest, PROTECTED_GeneratesNrProofWithCredential_EPS0) {
  PrivKey mpriv_key = this->kEps0MemberPrivateKey;
  GroupPubKey pub_key = this->kEps0GroupPublicKey;
  Prng my_prng;
  MemberCtxObj member(pub_key, *(MembershipCredential const*)&mpriv_key,
                      &Prng::Generate, &my_prng);

  BasicSignature basic_sig;
  auto msg = this->kTest1Msg;
  SigRl const* sig_rl =
      reinterpret_cast<const SigRl*>(this->kEps0GroupSigRl.data());
  BigNumStr rnd_bsn = {0};

  SplitNrProof proof;
  FpElemStr nonce_k;

  ASSERT_EQ(kEpidNoErr,
            EpidSplitSignBasic(member, msg.data(), msg.size(), nullptr, 0,
                               &basic_sig, &rnd_bsn, &nonce_k));
  EXPECT_EQ(kEpidNoErr,
            EpidNrProve(member, msg.data(), msg.size(), &rnd_bsn,
                        sizeof(rnd_bsn), &basic_sig, &sig_rl->bk[0], &proof));

  // verify signature
  VerifierCtxObj verifier(pub_key);
  EXPECT_EQ(kEpidSigValid,
            EpidNrVerify(verifier, &basic_sig, msg.data(), msg.size(),
                         &sig_rl->bk[0], &proof, sizeof(proof)));
}

}  // namespace