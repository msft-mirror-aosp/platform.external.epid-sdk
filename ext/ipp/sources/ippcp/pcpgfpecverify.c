/*******************************************************************************
* Copyright 2010-2018 Intel Corporation
* All Rights Reserved.
*
* If this  software was obtained  under the  Intel Simplified  Software License,
* the following terms apply:
*
* The source code,  information  and material  ("Material") contained  herein is
* owned by Intel Corporation or its  suppliers or licensors,  and  title to such
* Material remains with Intel  Corporation or its  suppliers or  licensors.  The
* Material  contains  proprietary  information  of  Intel or  its suppliers  and
* licensors.  The Material is protected by  worldwide copyright  laws and treaty
* provisions.  No part  of  the  Material   may  be  used,  copied,  reproduced,
* modified, published,  uploaded, posted, transmitted,  distributed or disclosed
* in any way without Intel's prior express written permission.  No license under
* any patent,  copyright or other  intellectual property rights  in the Material
* is granted to  or  conferred  upon  you,  either   expressly,  by implication,
* inducement,  estoppel  or  otherwise.  Any  license   under such  intellectual
* property rights must be express and approved by Intel in writing.
*
* Unless otherwise agreed by Intel in writing,  you may not remove or alter this
* notice or  any  other  notice   embedded  in  Materials  by  Intel  or Intel's
* suppliers or licensors in any way.
*
*
* If this  software  was obtained  under the  Apache License,  Version  2.0 (the
* "License"), the following terms apply:
*
* You may  not use this  file except  in compliance  with  the License.  You may
* obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
*
*
* Unless  required  by   applicable  law  or  agreed  to  in  writing,  software
* distributed under the License  is distributed  on an  "AS IS"  BASIS,  WITHOUT
* WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
* See the   License  for the   specific  language   governing   permissions  and
* limitations under the License.
*******************************************************************************/

/*
//     Intel(R) Integrated Performance Primitives. Cryptography Primitives.
//     EC over GF(p^m) definitinons
//
//     Context:
//        ippsGFpECVerify()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpgfpecstuff.h"
#include "pcpeccp.h"

//tbcd: temporary excluded: #include <assert.h>
/*F*
// Name: ippsGFpECVerify
//
// Purpose: Verifies the parameters of an elliptic curve.
//
// Returns:                   Reason:
//    ippStsNullPtrErr          pEC == NULL
//                              pResult == NULL
//                              pScratchBuffer == NULL
//    ippStsContextMatchErr     invalid pEC->idCtx
//    ippStsNoErr               no error
//
// Parameters:
//    pResult         Pointer to the verification result
//    pEC             Pointer to the context of the elliptic curve
//    pScratchBuffer  Pointer to the scratch buffer
//
*F*/

IPPFUN(IppStatus, ippsGFpECVerify,(IppECResult* pResult, IppsGFpECState* pEC, Ipp8u* pScratchBuffer))
{
   IPP_BAD_PTR3_RET(pEC, pResult, pScratchBuffer);
   pEC = (IppsGFpECState*)( IPP_ALIGNED_PTR(pEC, ECGFP_ALIGNMENT) );
   IPP_BADARG_RET( !ECP_TEST_ID(pEC), ippStsContextMatchErr );

   *pResult = ippECValid;

   {
      IppsGFpState* pGF = ECP_GFP(pEC);
      gsModEngine* pGFE = GFP_PMA(pGF);
      int elemLen = GFP_FELEN(pGFE);

      mod_mul mulF = GFP_METHOD(pGFE)->mul;
      mod_sqr sqrF = GFP_METHOD(pGFE)->sqr;
      mod_add addF = GFP_METHOD(pGFE)->add;

      /*
      // check discriminant ( 4*A^3 + 27*B^2 != 0 mod P)
      */
      if(ippECValid == *pResult) {
         BNU_CHUNK_T* pT = cpGFpGetPool(1, pGFE);
         BNU_CHUNK_T* pU = cpGFpGetPool(1, pGFE);
         //tbcd: temporary excluded: assert(NULL!=pT && NULL!=pU);

         if(ECP_SPECIFIC(pEC)==ECP_EPID2)
            cpGFpElementPadd(pT, elemLen, 0);         /* T = 4*A^3 = 0 */
         else {
            addF(pT, ECP_A(pEC), ECP_A(pEC), pGFE);   /* T = 4*A^3 */
            sqrF(pT, pT, pGFE);
            mulF(pT, ECP_A(pEC), pT, pGFE);
         }

         addF(pU, ECP_B(pEC), ECP_B(pEC), pGFE);      /* U = 9*B^2 */
         addF(pU, pU, ECP_B(pEC), pGFE);
         sqrF(pU, pU, pGFE);

         addF(pT, pU, pT, pGFE);                      /* T += 3*U */
         addF(pT, pU, pT, pGFE);
         addF(pT, pU, pT, pGFE);

         *pResult = GFP_IS_ZERO(pT, elemLen)? ippECIsZeroDiscriminant: ippECValid;

         cpGFpReleasePool(2, pGFE);
      }

      if(ECP_SUBGROUP(pEC)) {
         /*
         // check base point and it order
         */
         if(ippECValid == *pResult) {
            IppsGFpECPoint G;
            cpEcGFpInitPoint(&G, ECP_G(pEC), ECP_AFFINE_POINT|ECP_FINITE_POINT, pEC);

            /* check G != infinity */
            *pResult = gfec_IsPointAtInfinity(&G)? ippECPointIsAtInfinite : ippECValid;

            /* check G lies on EC */
            if(ippECValid == *pResult)
               *pResult = gfec_IsPointOnCurve(&G, pEC)? ippECValid : ippECPointIsNotValid;

            /* check Gorder*G = infinity */
            if(ippECValid == *pResult) {
               IppsGFpECPoint T;
               cpEcGFpInitPoint(&T, cpEcGFpGetPool(1, pEC),0, pEC);

               gfec_MulBasePoint(&T, MOD_MODULUS(ECP_MONT_R(pEC)), BITS_BNU_CHUNK(ECP_ORDBITSIZE(pEC)), pEC, pScratchBuffer);

               *pResult = gfec_IsPointAtInfinity(&T)? ippECValid : ippECInvalidOrder;

               cpEcGFpReleasePool(1, pEC);
            }
         }

         /*
         // check order==P
         */
         if(ippECValid == *pResult) {
            BNU_CHUNK_T* pPrime = GFP_MODULUS(pGFE);
            int primeLen = GFP_FELEN(pGFE);

            gsModEngine* pR = ECP_MONT_R(pEC);
            BNU_CHUNK_T* pOrder = MOD_MODULUS(pR);
            int orderLen = MOD_LEN(pR);

            *pResult = (primeLen==orderLen && GFP_EQ(pPrime, pOrder, primeLen))? ippECIsWeakSSSA : ippECValid;
         }
      }

      return ippStsNoErr;
   }
}
