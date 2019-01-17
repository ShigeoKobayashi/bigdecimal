/*
 *
 * Bigdecimal(Variable decimal precision) C/C++ library.
 *
 * Copyright(C) 2012 by Shigeo Kobayashi(shigeo@tinyforest.jp)
 * 
 *    Version 1.1
 */

#include "stdafx.h"

/* Version 1.1       */
/*   VpLoad() added. */
VP_EXPORT(int) VpVersion() {return 11;}

/*
 Maximum iteration count for VpSqrt() etc.
*/
static VP_UINT  gMaxIter   = 10000UL;
static VP_UINT  gIterCount = 0; /* Recent iteration count */

/* following constants are computed in VpInit()    */
static VP_UINT  BASE_FIG = 4;        /* =log10(BASE)  */
static VP_DIGIT BASE     = 10000L;   /* Base value(value must be 10**BASE_FIG) */
                                     /* The value of BASE**2 + BASE must be represented */
                                     /* within one VP_DIGIT. */
static VP_DIGIT HALF_BASE = 0; /* = (BASE/2)       */
static VP_DIGIT BASE1     = 0; /* = (BASE/10)      */
static VP_UINT  DBLE_FIG  = 0; /* = (DBL_DIG+1): figure of double */
static Real    *VpConstOne;    /* constant  1.0    */
static Real    *VpConstNOne;   /* constant -1.0    */
static Real    *VpConstTwo;    /* constant  2.0    */
static Real    *VpConstPt5;    /* constant  0.5    */

static int     gAllocCount     = 0;                /* VP_HANDLE allocation count */
static int     gRoundMode      = VP_ROUND_HALF_UP; /* Default rounding mode */

/* Print options for VP_HANDLE */
static VP_UINT gDigitSeparationCount = 10;
static char    gDigitSeparator = ' ';
static char    gDigitLeader    = ' '; /* must be 0 or 
                                       '+' ... '+' is printed for posotive number.
                                       ' ' ...  ' ' is printed for positive number.
                                       */

/*
   ================================================
   CInitializer() initializes this DLL when loaded.
   ================================================
*/
static void VpInit();
class VpInitializer
{
public:
    VpInitializer()
    {
#ifdef _DEBUG
        printf("\n------VpInitializer()-----\n");
#endif
        VpInit();
    };
    ~VpInitializer()
    {
#ifdef _DEBUG
        printf("~VpInitializer: gAllocCount=%d\n",gAllocCount);
#endif
    };
};
static VpInitializer *gInit = new VpInitializer();
// ================================================

/*
  Exception handler.
*/
static VP_EXCEPTION_HANDLER *gpExceptionHandler = NULL;
VP_EXPORT(void)
    VpSetExceptionHandler(VP_EXCEPTION_HANDLER *pf)
{
    gpExceptionHandler = pf;
}
VP_EXPORT(VP_UINT) 
    VpGetTag(VP_HANDLE h)
{
    return ((Real*)h)->Tag;
}

VP_EXPORT(void)
    VpSetTag(VP_HANDLE h,VP_UINT tag)
{
    ((Real*)h)->Tag = tag;
}

#define VpException(a,b) VpExceptionHandler(a,b,__FILE__,__LINE__)
static VP_HANDLE
    VpExceptionHandler(VP_HANDLE h,const char *pszMsg,const char *pszFile,int line)
{
#ifdef _DEBUG
    printf("VpException: message=%s at %d (%s)\n",pszMsg,line,pszFile);
#endif
    if(gpExceptionHandler!=NULL) (*gpExceptionHandler)(h,pszMsg);
    return h;
}

static int
    VpHasVal(VP_HANDLE a)
{
    return ((Real*)a)->frac[0]!=0;
}


/*
   Allocate empty Real(==0.0)
   mx ... number of digits the allocated Real can have.
*/
VP_EXPORT(VP_HANDLE)
    VpMemAlloc(VP_UINT m)
{
    int    mx  = (m+BASE_FIG-1)/BASE_FIG + 1; 
    VP_UINT s  = sizeof(Real)+mx*sizeof(VP_DIGIT); /* frac[] has mx+1 elements(intensional) */
    Real  *p   = (Real*)malloc(s);

    ASSERT(p!=NULL);
    if(!p) {
        char sz[128];
        sprintf(sz,"Memory allocation error(%lu)!",(unsigned long)mx);
        return VpException(VP_ERROR_MEMORY_ALLOCATION,sz);
    }
    memset(p,0,s);
    p->MaxPrec = mx;
    p->Size    = s;
    VpSetPosZero((VP_HANDLE)p);
    gAllocCount++; /* Count up */
    return (VP_HANDLE)p;
}

/*
  If exponent overflows then 0
  or returns 1.
*/
static int
    AddExponent(Real *a, int n)
{
    int   e = a->exponent;
    int   m = e+n;
    int   eb, mb;
    if(e>0) {
        if(n>0) {
            mb = m*(int  )BASE_FIG;
            eb = e*(int  )BASE_FIG;
            if(mb<eb) goto overflow;
        }
    } else if(n<0) {
        mb = m*(int  )BASE_FIG;
        eb = e*(int  )BASE_FIG;
        if(mb>eb) goto underflow;
    }
    a->exponent = m;
    return 1;

/* Overflow/Underflow ==> Raise exception or returns 0 */
underflow:
    VpException(VpSetZero((VP_HANDLE)a,VP_SIGN(a)),"Exponent underflow!");
    return 0;

overflow:
    VpException(VpSetInf((VP_HANDLE)a,VP_SIGN(a)),"Exponent overflow!");;
    return 0;
}

/*
 *  Input  a = 00000xxxxxxxx En(5 preceeding zeros)
 *  Output a = xxxxxxxx En-5
 */
static VP_HANDLE
    VpNmlz(Real *a)
{
    VP_UINT ind_a, i;

    if (!VpIsNumeric((VP_HANDLE)a)) goto NoVal;

    /* Not NaN nor Inf */
    ind_a = a->Prec;
    while (ind_a--) {
        if (a->frac[ind_a]) {
            a->Prec = ind_a + 1;
            i = 0;
            while (a->frac[i] == 0) ++i;        /* skip the first few zeros */
            if (i) {
                a->Prec -= i;
                if (!AddExponent(a, -(int)i)) return (VP_HANDLE)a;
                memmove(&a->frac[0], &a->frac[i], a->Prec*sizeof(VP_DIGIT));
            }
            return (VP_HANDLE)a;
        }
    }
    /* a is zero(no non-zero digit) */
    VpSetZero((VP_HANDLE)a, VP_SIGN(a));
    return (VP_HANDLE)a;

NoVal:
    a->frac[0] = 0;
    a->Prec = 1;
    return (VP_HANDLE)a;
}

/*
 *  Rounds up m(plus one to final digit of m).
 */
static int
    VpRdup(Real *m, VP_UINT ind_m)
{
    int  carry;

    if (!ind_m) ind_m = m->Prec;

    carry = 1;
    while (carry > 0 && (ind_m--)) {
        m->frac[ind_m] += carry;
        if (m->frac[ind_m] >= BASE) m->frac[ind_m] -= BASE;
        else                        carry = 0;
    }
    if(carry > 0) {        /* Overflow,count exponent and set fraction part be 1  */
        if (!AddExponent(m, 1)) return 0;
        m->Prec    = 1;
        m->frac[0] = 1;
    } else {
        VpNmlz(m);
    }
    return 1;
}

static void VpInternalRound2(Real *c, int ixDigit, VP_DIGIT vPrev, VP_DIGIT v,int RoundMode);
static void
    VpInternalRound(Real *c, int ixDigit, VP_DIGIT vPrev, VP_DIGIT v)
{
    VpInternalRound2(c,ixDigit,vPrev,v,VpGetRoundMode());
}

static void
    VpInternalRound2(Real *c, int ixDigit, VP_DIGIT vPrev, VP_DIGIT v,int RoundMode)
{
    int f = 0;

    if (!v) goto Exit;

    v /= BASE1;
    switch (RoundMode)
    {
    case VP_ROUND_DOWN:
        break;
    case VP_ROUND_UP:
        if (v) f = 1;
        break;
    case VP_ROUND_HALF_UP:
        if (v >= 5) f = 1;
        break;
    case VP_ROUND_HALF_DOWN:
        /*    this is ok - because this is the last digit of precision,
            the case where v == 5 and some further digits are nonzero
            will never occur */
        if (v >= 6) f = 1;
        break;
    case VP_ROUND_CEIL:
        if (v && (VP_SIGN(c) > 0)) f = 1;
        break;
    case VP_ROUND_FLOOR:
        if (v && (VP_SIGN(c) < 0)) f = 1;
        break;
    case VP_ROUND_HALF_EVEN:  /* Banker's rounding */
        /* as per VP_ROUND_HALF_DOWN, because this is the last digit of precision,
        there is no case to worry about where v == 5 and some further digits are nonzero */
        if (v > 5) f = 1;
        else if (v == 5 && vPrev % 2) f = 1;
        break;
    }

    if (f) VpRdup(c, ixDigit);

Exit:
    VpNmlz(c);
}

/*
 check if the result of c = a op b,
    if the result is not NaN or Infinity then returns 1.
    otherwise (NaN or Infinity),returns 0.
*/
static int 
    VpIsNumericOP(Real *c,Real *a,Real *b,char op)
{
    if((VP_HANDLE)c==(VP_HANDLE)a ||
       (VP_HANDLE)c==(VP_HANDLE)b) {
        VpException(VpSetNaN((VP_HANDLE)c),"Invalid operation(input==output)!");
        return 0;
    }

    if(VpIsNaN((VP_HANDLE)a) || VpIsNaN((VP_HANDLE)b)) {
        /* at least a or b is NaN */
        VpException(VpSetNaN((VP_HANDLE)c),"Operation includes NaN!");
        return 0;
    }

    if(VpIsInf((VP_HANDLE)a)) {
        // a == Infinity
        if(VpIsInf((VP_HANDLE)b)) {
            switch(op)
            {
            case '+': /* + */
                if(VP_SIGN(a)==VP_SIGN(b)) {
                    // Int + Inf(same sign)
                    VpException(VpSetInf((VP_HANDLE)c,VP_SIGN(a)),"Operation includes Infinity!");
                    return 0;
                } else {
                    // Inf + Inf (different sign)
                    VpException(VpSetNaN((VP_HANDLE)c),"Result NaN(Inf - Inf)!");
                    return 0;;
                }
            case '-': /* - */
                if(VP_SIGN(a)!=VP_SIGN(b)) {
                    // Inf - Inf(different sign)
                    VpException(VpSetInf((VP_HANDLE)c,VP_SIGN(a)),"Result Infinity(Inf + Inf)!");
                    return 0;
                } else {
                    // Inf - Inf (same sign)
                    VpException(VpSetNaN((VP_HANDLE)c),"Result NaN(Inf + Inf)!");
                    return 0;
                }
                break;
            case '*': /* * */
                // Inf * Inf
                VpException(VpSetInf((VP_HANDLE)c,VP_SIGN(a)*VP_SIGN(b)),"Result Infinity(Inf * Inf)!");
                return 0;
            case '/': /* / */
                // Inf / Inf
                VpException(VpSetNaN((VP_HANDLE)c),"Result NaN(Inf / Inf)!");
                return 0;
            }
            // ??
            ASSERT(FALSE);
            VpException(VpSetNaN((VP_HANDLE)c),"Result NaN(undefined)!");
            return 0;
        }

        /* Inf op Finite */
        switch(op)
        {
        case '+': /* + */
        case '-': /* - */
                VpException(VpSetInf((VP_HANDLE)c,VP_SIGN(a)),"Result Infinity(Inf included)!");
                return 0;
        case '*': /* * */
                if(VpIsZero((VP_HANDLE)b)) {
                    // Inf * 0 
                    VpException(VpSetNaN((VP_HANDLE)c),"Result NaN(Inf * 0)!");
                    return 0;
                }
                // Inf * x
                VpException(VpSetInf((VP_HANDLE)c,VP_SIGN(a)*VP_SIGN(b)),"Result Infinity(Inf * x)!");
                return 0;
        case '/': /* / */
                // Inf / X
                if(VpIsZero((VP_HANDLE)b)) {
                    // Inf * 0 
                    VpException(VpSetNaN((VP_HANDLE)c),"Result NaN(Inf * 0)!");
                    return 0;
                }
                VpException(VpSetInf((VP_HANDLE)c,VP_SIGN(a)*VP_SIGN(b)),"Result Infinity(Inf * x)!");
                return 0;
        }
        // ???
        ASSERT(FALSE);
        VpException(VpSetInf((VP_HANDLE)c,VP_SIGN(a)*VP_SIGN(b)),"Result Infinity(undefined)!");
        return 0;
    }

    if(VpIsInf((VP_HANDLE)b)) {
        // a is finite b is Infinity
        switch(op)
        {
        case '+': /* + */
                VpException(VpSetInf((VP_HANDLE)c,VP_SIGN(b)),"Result Infinity(Inf + x)!");
                return 0;
        case '-': /* - */
                VpException(VpSetInf((VP_HANDLE)c,-VP_SIGN(b)),"Result Infinity(Inf - x)!");
                return 0;
        case '*': /* * */
                if(VpIsZero((VP_HANDLE)a)) {
                    VpException(VpSetNaN((VP_HANDLE)c),"Result NaN(Inf * 0)!");
                    return 0;
                }
                VpException(VpSetInf((VP_HANDLE)c,VP_SIGN(a)*VP_SIGN(b)),"Result Infinity(Inf * x)!");
                return 0;
        case '/': /* / */
                VpException(VpSetZero((VP_HANDLE)c,VP_SIGN(a)*VP_SIGN(b)),"Result zero(x/Inf)!");
                return 0;
        }
        // ???
        ASSERT(FALSE);
        VpException(VpSetInf((VP_HANDLE)c,VP_SIGN(a)*VP_SIGN(b)),"Result Infinity(undefined)!");
        return 0;
    }

    if(op=='/' && VpIsZero((VP_HANDLE)b)) {
        /* Zero devide !! */
        if(VpIsZero((VP_HANDLE)a)) {
            VpException(VpSetNaN((VP_HANDLE)c),"Result NaN (0/0)!");
        } else {
            VpException(VpSetInf((VP_HANDLE)c,VP_SIGN(a)*VP_SIGN(b)),"Result Infinity(zero division)!");
        }
        return 0;
    }
    return 1; /* Results OK */
}


/*
 * Note: If(av+bv)>= HALF_BASE,then 1 will be added to the least significant
 *    digit of c(In case of addition).
 * ------------------------- figure of output -----------------------------------
 *      a =  xxxxxxxxxxx
 *      b =    xxxxxxxxxx
 *      c =xxxxxxxxxxxxxxx
 *      word_shift =  |   |
 *      right_word =  |    | (Total digits in RHSV)
 *      left_word  = |   |   (Total digits in LHSV)
 *      a_pos      =    |
 *      b_pos      =     |
 *      c_pos      =      |
 */
static VP_UINT 
    VpSetPTR(Real *a, Real *b, Real *c, VP_UINT *a_pos, VP_UINT *b_pos, VP_UINT *c_pos, VP_DIGIT *av, VP_DIGIT *bv)
{
    VP_UINT left_word, right_word, word_shift;
    c->frac[0] = 0;
    *av = *bv = 0;
    word_shift =((a->exponent) -(b->exponent));
    left_word = b->Prec + word_shift;
    right_word = Max((a->Prec),left_word);
    left_word =(c->MaxPrec) - 1;    /* -1 ... prepare for round up */
    /*
     * check if 'round' is needed.
     */
    if(right_word > left_word) {    /* round ? */
        /*---------------------------------
         *  Actual size of a = xxxxxxAxx
         *  Actual size of b = xxxBxxxxx
         *  Max. size of   c = xxxxxx
         *  Round off        =   |-----|
         *  c_pos            =   |
         *  right_word       =   |
         *  a_pos            =    |
         */
        *c_pos = right_word = left_word + 1;    /* Set resulting precision */
        /* be equal to that of c */
        if((a->Prec) >=(c->MaxPrec)) {
            /*
             *   a =  xxxxxxAxxx
             *   c =  xxxxxx
             *   a_pos =    |
             */
            *a_pos = left_word;
            *av = a->frac[*a_pos];    /* av is 'A' shown in above. */
        } else {
            /*
             *   a = xxxxxxx
             *   c = xxxxxxxxxx
             *  a_pos =     |
             */
            *a_pos = a->Prec;
        }
        if((b->Prec + word_shift) >= c->MaxPrec) {
            /*
             *   a = xxxxxxxxx
             *   b =  xxxxxxxBxxx
             *   c = xxxxxxxxxxx
             *  b_pos =   |
             */
            if(c->MaxPrec >=(word_shift + 1)) {
                *b_pos = c->MaxPrec - word_shift - 1;
                *bv = b->frac[*b_pos];
            } else {
                *b_pos = -1L;
            }
        } else {
            /*
             *   a = xxxxxxxxxxxxxxxx
             *   b =  xxxxxx
             *   c = xxxxxxxxxxxxx
             *  b_pos =     |
             */
            *b_pos = b->Prec;
        }
    } else {            /* The MaxPrec of c - 1 > The Prec of a + b  */
        /*
         *    a =   xxxxxxx
         *    b =   xxxxxx
         *    c = xxxxxxxxxxx
         *   c_pos =   |
         */
        *b_pos = b->Prec;
        *a_pos = a->Prec;
        *c_pos = right_word + 1;
    }
    c->Prec = *c_pos;
    c->exponent = a->exponent;
    if(!AddExponent(c,1)) return (VP_UINT)-1L;
    return word_shift;
}

/*
 * Addition of two variable precisional variables
 * a and b assuming abs(a)>abs(b).
 *   c = abs(a) + abs(b) ; where |a|>=|b|
 */
static int
    VpAddAbs(Real *a, Real *b, Real *c)
{
    VP_UINT    word_shift;
    VP_UINT    ap;
    VP_UINT    bp;
    VP_UINT    cp;
    VP_UINT    a_pos;
    VP_UINT    b_pos, b_pos_with_word_shift;
    VP_UINT    c_pos;
    VP_DIGIT  av, bv, carry, mrv;

    word_shift = VpSetPTR(a, b, c, &ap, &bp, &cp, &av, &bv);
    a_pos = ap;
    b_pos = bp;
    c_pos = cp;
    if(word_shift==(VP_UINT)-1L) return 0; /* Overflow */
    if(b_pos == (VP_UINT)-1L) goto Assign_a;

    mrv = av + bv; /* Most right val. Used for round. */

    /* Just assign the last few digits of b to c because a has no  */
    /* corresponding digits to be added. */
    while(b_pos + word_shift > a_pos) {
        --c_pos;
        if(b_pos > 0) {
            c->frac[c_pos] = b->frac[--b_pos];
        } else {
            --word_shift;
            c->frac[c_pos] = 0;
        }
    }

    /* Just assign the last few digits of a to c because b has no */
    /* corresponding digits to be added. */
    b_pos_with_word_shift = b_pos + word_shift;
    while(a_pos > b_pos_with_word_shift) {
        c->frac[--c_pos] = a->frac[--a_pos];
    }
    carry = 0;    /* set first carry be zero */

    /* Now perform addition until every digits of b will be */
    /* exhausted. */
    while(b_pos > 0) {
        c->frac[--c_pos] = a->frac[--a_pos] + b->frac[--b_pos] + carry;
        if(c->frac[c_pos] >= BASE) {
            c->frac[c_pos] -= BASE;
            carry = 1;
        } else {
            carry = 0;
        }
    }

    /* Just assign the first few digits of a with considering */
    /* the carry obtained so far because b has been exhausted. */
    while(a_pos > 0) {
        c->frac[--c_pos] = a->frac[--a_pos] + carry;
        if(c->frac[c_pos] >= BASE) {
            c->frac[c_pos] -= BASE;
            carry = 1;
        } else {
            carry = 0;
        }
    }
    if(c_pos) c->frac[c_pos - 1] += carry;
    VpNmlz(c);
    goto Exit;

Assign_a:
    VpAsgn((VP_HANDLE)c, (VP_HANDLE)a, 1);
    mrv = 0;

Exit:

    return (int)mrv;
}

/*
 * c = abs(a) - abs(b)
 */
static int 
    VpSubAbs(Real *a, Real *b, Real *c)
{
    VP_UINT   word_shift;
    VP_UINT   ap;
    VP_UINT   bp;
    VP_UINT   cp;
    VP_UINT   a_pos;
    VP_UINT   b_pos, b_pos_with_word_shift;
    VP_UINT   c_pos;
    VP_DIGIT av, bv, mrv;
    VP_DIGIT borrow;

    word_shift = VpSetPTR(a, b, c, &ap, &bp, &cp, &av, &bv);
    a_pos = ap;
    b_pos = bp;
    c_pos = cp;
    if(word_shift==(VP_UINT)-1L) return 0; /* Overflow */
    if(b_pos == (VP_UINT)-1L) goto Assign_a;

    if(av >= bv) {
        mrv = av - bv;
        borrow = 0;
    } else {
        mrv    = 0;
        borrow = 1;
    }

    /* Just assign the values which are the BASE subtracted by   */
    /* each of the last few digits of the b because the a has no */
    /* corresponding digits to be subtracted. */
    if(b_pos + word_shift > a_pos) {
        while(b_pos + word_shift > a_pos) {
            --c_pos;
            if(b_pos > 0) {
                c->frac[c_pos] = BASE - b->frac[--b_pos] - borrow;
            } else {
                --word_shift;
                c->frac[c_pos] = BASE - borrow;
            }
            borrow = 1;
        }
    }
    /* Just assign the last few digits of a to c because b has no */
    /* corresponding digits to subtract. */

    b_pos_with_word_shift = b_pos + word_shift;
    while(a_pos > b_pos_with_word_shift) {
        c->frac[--c_pos] = a->frac[--a_pos];
    }

    /* Now perform subtraction until every digits of b will be */
    /* exhausted. */
    while(b_pos > 0) {
        --c_pos;
        if(a->frac[--a_pos] < b->frac[--b_pos] + borrow) {
            c->frac[c_pos] = BASE + a->frac[a_pos] - b->frac[b_pos] - borrow;
            borrow = 1;
        } else {
            c->frac[c_pos] = a->frac[a_pos] - b->frac[b_pos] - borrow;
            borrow = 0;
        }
    }

    /* Just assign the first few digits of a with considering */
    /* the borrow obtained so far because b has been exhausted. */
    while(a_pos > 0) {
        --c_pos;
        if(a->frac[--a_pos] < borrow) {
            c->frac[c_pos] = BASE + a->frac[a_pos] - borrow;
            borrow = 1;
        } else {
            c->frac[c_pos] = a->frac[a_pos] - borrow;
            borrow = 0;
        }
    }
    if(c_pos) c->frac[c_pos - 1] -= borrow;
    VpNmlz(c);
    goto Exit;

Assign_a:
    VpAsgn((VP_HANDLE)c, (VP_HANDLE)a, 1);
    mrv = 0;

Exit:
    return (int)mrv;
}


//
// operation ... +1 addition
//               -1 subtruction
// returns c
//
static VP_HANDLE
    VpAddSub(Real *c, Real *a, Real *b, int operation)
{
    short isw;
    Real *a_ptr, *b_ptr;
    VP_UINT n, na, nb, i;
    VP_DIGIT  mrv;
    char op = '+';
    if(operation<0) op = '-';
    if(!VpIsNumericOP(c,a,b,op)) goto Exit;
    
    /* check if a or b is zero  */
    if(VpIsZero((VP_HANDLE)a)) {
        /* a is zero,then assign b to c */
        if(!VpIsZero((VP_HANDLE)b)) {
            VpAsgn((VP_HANDLE)c, (VP_HANDLE)b, operation);
        } else {
            /* Both a and b are zero. */
            if(VP_SIGN(a)<0 && operation*VP_SIGN(b)<0) {
                /* -0 -0 */
                VpSetZero((VP_HANDLE)c,-1);
            } else {
                VpSetZero((VP_HANDLE)c,1);
            }
        }
        goto Exit;
    }

    if(VpIsZero((VP_HANDLE)b)) {
        /* b is zero,then assign a to c. */
        VpAsgn((VP_HANDLE)c, (VP_HANDLE)a, 1);
        goto Exit;
    }

    /* compare absolute value. As a result,|a_ptr|>=|b_ptr| */
    if(a->exponent > b->exponent) {
        a_ptr = a;
        b_ptr = b;
    }         /* |a|>|b| */
    else if(a->exponent < b->exponent) {
        a_ptr = b;
        b_ptr = a;
    }                /* |a|<|b| */
    else {
        /* Exponent part of a and b is the same,then compare fraction */
        /* part */
        na = a->Prec;
        nb = b->Prec;
        n = Min(na, nb);
        for(i=0;i < n; ++i) {
            if(a->frac[i] > b->frac[i]) {
                a_ptr = a;
                b_ptr = b;
                goto end_if;
            } else if(a->frac[i] < b->frac[i]) {
                a_ptr = b;
                b_ptr = a;
                goto end_if;
            }
        }
        if(na > nb) {
         a_ptr = a;
            b_ptr = b;
            goto end_if;
        } else if(na < nb) {
            a_ptr = b;
            b_ptr = a;
            goto end_if;
        }
        /* |a| == |b| */
        if(VP_SIGN(a) + operation *VP_SIGN(b) == 0) {
            VpSetZero((VP_HANDLE)c,1);        /* abs(a)=abs(b) and operation = '-'  */
            goto Exit;
        }
        a_ptr = a;
        b_ptr = b;
    }

end_if:
    isw = VP_SIGN(a) + operation *VP_SIGN(b);
    /*
     *  isw = 0 ...( 1)+(-1),( 1)-( 1),(-1)+(1),(-1)-(-1)
     *      = 2 ...( 1)+( 1),( 1)-(-1)
     *      =-2 ...(-1)+(-1),(-1)-( 1)
     *   If isw==0, then c =(Sign a_ptr)(|a_ptr|-|b_ptr|)
     *              else c =(Sign ofisw)(|a_ptr|+|b_ptr|)
    */
    if(isw) {            /* addition */
        VP_FINITE_SIGN(c,1);
        mrv = VpAddAbs(a_ptr, b_ptr, c);
        VP_FINITE_SIGN(c, isw / 2);
    } else {            /* subtraction */
        VP_FINITE_SIGN(c,1);
        mrv = VpSubAbs(a_ptr, b_ptr, c);
        if(a_ptr == a) {
            VP_SIGN(c) = VP_SIGN(a);
        } else    {
            VP_SIGN(c) = VP_SIGN(a_ptr) * operation;
        }
    }
    VpInternalRound(c,0,(c->Prec>0)?c->frac[c->Prec-1]:0,mrv);

Exit:
    return (VP_HANDLE)c;
}

/*
 * Initializer Vp routines and constants used.
 */
static void 
    VpInit()
{
	/*
	  BASE must be 10**n  (n=BASE_FIG)
	  And BASE*BASE must be retresented by 1 VP_DIGIT.
	  To check BASE*BASE is within VP_DIGIT, BASE-((BASE)*(BASE))/BASE == 0.
	  If BASE*BASE overflows BASE-((BASE)*(BASE))/BASE will not be zero.
	  -------------------------
	  VP_DIGIT v;
 	  v        = 10;
	  BASE_FIG = 0;
	  while((v-((v*v)/v))==0) {
		  BASE = v;
		  BASE_FIG++;
		  v   *= 10;
	  }
	  -------------------------
   */

    ASSERT(sizeof(VP_DIGIT)==4 || sizeof(VP_DIGIT)==8);
    if(sizeof(VP_DIGIT)==4) {
        /* 32-bit */ 
        BASE_FIG = 4;       /* =log10(BASE)  */
        BASE     = 10000UL; /* Base value(value must be 10**BASE_FIG) */
    }
    if(sizeof(VP_DIGIT)==8) {
        /* 64-bit */ 
        BASE_FIG = 9;
        BASE     = 1000000000ULL;
    }
#ifdef _DEBUG
    printf("VpInit: BASE_FIG=%lu BASE=%llu\n",(VP_UINT)BASE_FIG,(unsigned long long)BASE);
#endif
    HALF_BASE = (BASE/2);
    BASE1     = (BASE/10);
    DBLE_FIG  = (DBL_DIG+1);    /* figure of double */

    /* Allocates Vp constants. */
    VpConstOne = (Real*)VpAlloc("1",1);
    VpConstPt5 = (Real*)VpAlloc("0.5",1);
	VpConstNOne= (Real*)VpAlloc("-1",1);
	VpConstTwo = (Real*)VpAlloc("2",1);

    gAllocCount = 0; /* Avobe 2 Reals are not counted. */
}


/*
 *
 *   c = a / b,  remainder = r
 *   Because 'r = a - c * b' must always be satisfied,
 *   And considering the case like c = 1/3 that the computation continue
 *   to c->MaxPrec.
 *   Following condition is mandatory,otherwise NaN may returned.
 *      r->MaxPrec > max(a->Prec,c->MaxPrec+b->Prec) 
 *
 */
VP_EXPORT(VP_HANDLE)
    VpDiv(VP_HANDLE hc, VP_HANDLE hr, VP_HANDLE ha, VP_HANDLE hb)
{
    Real *c = (Real*)hc;
    Real *r = (Real*)hr;
    Real *a = (Real*)ha;
    Real *b = (Real*)hb;

    VP_UINT    word_a, word_b, word_c, word_r;
    VP_UINT    i, n, ind_a, ind_b, ind_c, ind_r;
    VP_UINT    nLoop;
    VP_DIGIT  q, b1, b1p1, b1b2, b1b2p1, r1r2;
    VP_DIGIT  borrow, borrow1, borrow2;
    VP_DIGIT  qb;

    if(hr==hc||hr==ha||hr==hb) {
        return VpException(VpSetNaN(hc),"Invalid operation in VpDiv(input==output)!");
    }
    if(!VpIsNumericOP(c,a,b,'/')) return hc;

    VpSetNaN((VP_HANDLE)r);
    if(VpIsZero((VP_HANDLE)a)) {
        /* numerator a is zero  */
        VpSetZero((VP_HANDLE)c,VP_SIGN(a)*VP_SIGN(b));
        VpSetZero((VP_HANDLE)r,VP_SIGN(a)*VP_SIGN(b));
        return hc;
    }
    if(VpIsOne((VP_HANDLE)b)) {
        /* divide by one  */
        VpAsgn((VP_HANDLE)c, (VP_HANDLE)a, VP_SIGN(b));
        VpSetZero((VP_HANDLE)r,VP_SIGN(a));
        return hc;
    }

    word_a = a->Prec;
    word_b = b->Prec;
    word_c = c->MaxPrec;
    word_r = r->MaxPrec;

    ind_c = 0;
    ind_r = 1;
    /*
       c = a/b: a->Prec < r->MaxPrec
    */
    if(word_a >= word_r) goto space_error;

    r->frac[0] = 0;
    while(ind_r <= word_a) {
        r->frac[ind_r] = a->frac[ind_r - 1];
        ++ind_r;
    }

    while(ind_r < word_r) r->frac[ind_r++] = 0;
    while(ind_c < word_c) c->frac[ind_c++] = 0;

    /* initial procedure */
    b1 = b1p1 = b->frac[0];
    if(b->Prec <= 1) {
        b1b2p1 = b1b2 = b1p1 * BASE;
    } else {
        b1p1 = b1 + 1;
        b1b2p1 = b1b2 = b1 * BASE + b->frac[1];
        if(b->Prec > 2) ++b1b2p1;
    }

    /* */
    /* loop start */
    ind_c = word_r - 1;
    nLoop = Min(word_c,ind_c);
    ind_c = 1;
    while(ind_c < nLoop) {
        if(r->frac[ind_c] == 0) {
            ++ind_c;
            continue;
        }
        r1r2 = (int)r->frac[ind_c] * BASE + r->frac[ind_c + 1];
        if(r1r2 == b1b2) {
            /* The first two word digits is the same */
            ind_b = 2;
            ind_a = ind_c + 2;
            while(ind_b < word_b) {
                if(r->frac[ind_a] < b->frac[ind_b]) goto div_b1p1;
                if(r->frac[ind_a] > b->frac[ind_b]) break;
                ++ind_a;
                ++ind_b;
            }
            /* The first few word digits of r and b is the same and */
            /* the first different word digit of w is greater than that */
            /* of b, so quotinet is 1 and just subtract b from r. */
            borrow = 0;        /* quotient=1, then just r-b */
            ind_b = b->Prec - 1;
            ind_r = ind_c + ind_b;
            if(ind_r >= word_r) goto space_error;
            n = ind_b;
            for(i = 0; i <= n; ++i) {
                if(r->frac[ind_r] < b->frac[ind_b] + borrow) {
                    r->frac[ind_r] += (BASE - (b->frac[ind_b] + borrow));
                    borrow = 1;
                } else {
                    r->frac[ind_r] = r->frac[ind_r] - b->frac[ind_b] - borrow;
                    borrow = 0;
                }
                --ind_r;
                --ind_b;
            }
            ++c->frac[ind_c];
            goto carry;
        }
        /* The first two word digits is not the same, */
        /* then compare magnitude, and divide actually. */
        if(r1r2 >= b1b2p1) {
            q = r1r2 / b1b2p1;  /* q == (int )q  */
            c->frac[ind_c] += (int )q;
            ind_r = b->Prec + ind_c - 1;
            goto sub_mult;
        }

div_b1p1:
        if(ind_c + 1 >= word_c) goto out_side;
        q = r1r2 / b1p1;  /* q == (int )q */
        c->frac[ind_c + 1] += (int )q;
        ind_r = b->Prec + ind_c;

sub_mult:
        borrow1 = borrow2 = 0;
        ind_b = word_b - 1;
        if(ind_r >= word_r) goto space_error;
        n = ind_b;
        for(i = 0; i <= n; ++i) {
            /* now, perform r = r - q * b */
            qb = q * b->frac[ind_b];
            if (qb < BASE) borrow1 = 0;
            else {
                borrow1 = (int )(qb / BASE);
                qb -= (int)borrow1 * BASE;    /* get qb < BASE */
            }
            if(r->frac[ind_r] < qb) {
                r->frac[ind_r] += (int )(BASE - qb);
                borrow2 = borrow2 + borrow1 + 1;
            } else {
                r->frac[ind_r] -= (int )qb;
                borrow2 += borrow1;
            }
            if(borrow2) {
                if(r->frac[ind_r - 1] < borrow2) {
                    r->frac[ind_r - 1] += (BASE - borrow2);
                    borrow2 = 1;
                } else {
                    r->frac[ind_r - 1] -= borrow2;
                    borrow2 = 0;
                }
            }
            --ind_r;
            --ind_b;
        }

        r->frac[ind_r] -= borrow2;

carry:
        ind_r = ind_c;
        while(c->frac[ind_r] >= BASE) {
            c->frac[ind_r] -= BASE;
            --ind_r;
            ++c->frac[ind_r];
        }
    }

    /* End of operation, now final arrangement */
out_side:
    c->Prec = word_c;
    c->exponent = a->exponent;
    if(!AddExponent(c,2))              return hc;
    if(!AddExponent(c,-(b->exponent))) return hc;

    VP_FINITE_SIGN(c,VP_SIGN(a)*VP_SIGN(b));
    VpNmlz(c);            /* normalize c */
    r->Prec = word_r;
    r->exponent = a->exponent;
    if(!AddExponent(r,1)) return hc;
    VP_SIGN(r) = VP_SIGN(a);
    VpNmlz(r);            /* normalize r(remainder) */
    return hc;

space_error:
    ASSERT(FALSE);
    return VpException(VpSetNaN(hc),"Remainder's capacity too small in VpDiv()!");
}

/*
 *  VpComp = 0  ... if a=b,
 *    1  ... a>b,
 *   -1  ... a<b.
 *    9  ... result undefined(VP_ERROR_RESULT_NaN)
 */
VP_EXPORT(int)
    VpCmp(VP_HANDLE ha, VP_HANDLE hb)
{
    Real *a = (Real*)ha;
    Real *b = (Real*)hb;

    int     val;
    VP_UINT mx, ind;

    val = 0;
    if(VpIsNaN((VP_HANDLE)a)||VpIsNaN((VP_HANDLE)b)) return (int)VpException((VP_HANDLE)VP_ERROR_NON_NUMERIC,"Comparison contains NaN!");
    if(!VpIsNumeric((VP_HANDLE)a)) {
        if(!VpIsNumeric((VP_HANDLE)b)) {
            // a,b both Infinity
            if(a->sign>0 && b->sign<0) return  1;
            if(a->sign<0 && b->sign>0) return -1;
            return (int)VpException((VP_HANDLE)VP_ERROR_NON_NUMERIC,"Unable to compare Infinities!"); // unable to compare Infinity
        }
        if(a->sign>0)   return  1;
        return  -1;
    }
    if(!VpIsNumeric((VP_HANDLE)b)) {
        if(b->sign>0) return  -1;
        return 1;
    }
    /* Zero check */
    if(VpIsZero((VP_HANDLE)a)) {
        if(VpIsZero((VP_HANDLE)b))      return 0; /* both zero */
        val = -VP_SIGN(b);
        goto Exit;
    }
    if(VpIsZero((VP_HANDLE)b)) {
        val = VP_SIGN(a);
        goto Exit;
    }

    /* compare sign */
    if(VP_SIGN(a) > VP_SIGN(b)) {
        val = 1;        /* a>b */
        goto Exit;
    }
    if(VP_SIGN(a) < VP_SIGN(b)) {
        val = -1;        /* a<b */
        goto Exit;
    }

    /* a and b have same sign, && signe!=0,then compare exponent */
    if((a->exponent) >(b->exponent)) {
        val = VP_SIGN(a);
        goto Exit;
    }
    if((a->exponent) <(b->exponent)) {
        val = -VP_SIGN(b);
        goto Exit;
    }

    /* a and b have same exponent, then compare significand. */
    mx =((a->Prec) <(b->Prec)) ?(a->Prec) :(b->Prec);
    ind = 0;
    while(ind < mx) {
        if((a->frac[ind]) >(b->frac[ind])) {
            val = VP_SIGN(a);
         goto Exit;
        }
        if((a->frac[ind]) <(b->frac[ind])) {
            val = -VP_SIGN(b);
            goto Exit;
        }
        ++ind;
    }
    if((a->Prec) >(b->Prec)) {
        val = VP_SIGN(a);
    } else if((a->Prec) <(b->Prec)) {
        val = -VP_SIGN(b);
    }

Exit:
    if  (val> 1) val =  1;
    else if(val<-1) val = -1;

    return (int)val;
}

VP_EXPORT(int)
    VpExponent(VP_HANDLE h)
{
    Real    *a = (Real*)h;
    int      ex;
    VP_DIGIT n;

    if(!VpIsNumeric(h)) {
        VpException((VP_HANDLE)VP_ERROR_NON_NUMERIC,"Non numeric in VpExponent()");
		return 0;
    }
    if (!VpHasVal((VP_HANDLE)a)) return 0;

    ex = a->exponent * (int)BASE_FIG;
    n = BASE1;
    while ((a->frac[0] / n) == 0) {
         --ex;
         n /= 10;
    }
    return ex;
}

/*
 *  y = x - frac(x)
 */
VP_EXPORT(VP_HANDLE)
    VpFix(VP_HANDLE hy, VP_HANDLE hx)
{
    VP_UINT my, ind_y;
    Real *y = (Real*)hy;
    Real *x = (Real*)hx;

    if(!VpIsNumeric(hx))         return VpException(VpSetNaN(hy),"Non-numeric specified to VpFix()!");
    if(!VpHasVal((VP_HANDLE)x))  return VpAsgn((VP_HANDLE)y,(VP_HANDLE)x,1);
    if (x->exponent <= 0)        return VpSetZero((VP_HANDLE)y,VP_SIGN(x));

    /* satisfy: x->exponent > 0 */

    y->Prec = (VP_UINT)x->exponent;
    y->Prec = Min(y->Prec, y->MaxPrec);
    y->exponent = 0;
    VP_FINITE_SIGN(y,VP_SIGN(x));
    ind_y = 0;
    my = y->Prec;
    while(ind_y < my) {
        y->frac[ind_y] = x->frac[ind_y];
        y->exponent++;
        ++ind_y;
    }
    return VpNmlz(y);
}

/*
 *  y = x - fix(x)
 */
VP_EXPORT(VP_HANDLE)
    VpFrac(VP_HANDLE hy, VP_HANDLE hx)
{
    VP_UINT my, ind_y, ind_x;
    Real *y = (Real*)hy;
    Real *x = (Real*)hx;

    if(hy==hx) {
        return VpException(VpSetNaN(hy),"Invalid argument for VpFrac(y==x)!");
    }

    if(!VpIsNumeric(hx))                                    return VpException(VpSetNaN(hy),"Non-numeric specified to VpFrac()!");
    if(!VpHasVal((VP_HANDLE)x))                             return VpAsgn((VP_HANDLE)y,(VP_HANDLE)x,1);
    if (x->exponent > 0 && (VP_UINT)x->exponent >= x->Prec) return VpSetZero((VP_HANDLE)y,VP_SIGN(x));
    else if(x->exponent <= 0)                               return VpAsgn((VP_HANDLE)y, (VP_HANDLE)x, 1);

    /* satisfy: x->exponent > 0 */

    y->Prec = x->Prec - (VP_UINT)x->exponent;
    y->Prec = Min(y->Prec, y->MaxPrec);
    y->exponent = 0;
    VP_FINITE_SIGN(y,VP_SIGN(x));
    ind_y = 0;
    my = y->Prec;
    ind_x = x->exponent;
    while(ind_y < my) {
        y->frac[ind_y] = x->frac[ind_x];
        ++ind_y;
        ++ind_x;
    }
    return VpNmlz(y);
}

/*
 *   y = x ** n
 */
VP_EXPORT(VP_HANDLE)
    VpPower(VP_HANDLE hy, VP_HANDLE hx, int n)
{
    VP_UINT s, ss;
    int  sign;
    VP_HANDLE hw1 = 0;
    VP_HANDLE hw2 = 0;
    Real *w1 = NULL;
    Real *w2 = NULL;
    Real *x = (Real *)hx;
    Real *y = (Real *)hy;

    if(hy==hx) {
        return VpException(VpSetNaN(hy),"Invalid operation in VpPower(input==output)!");
    }

    if(VpIsZero((VP_HANDLE)x)) {
        if(n==0) return VpSetOne((VP_HANDLE)y);
        sign = VP_SIGN(x);
        if(n<0) {
           n = -n;
           if(sign<0) sign = (n%2)?(-1):(1);
           return VpSetInf((VP_HANDLE)y,sign);
        } else {
           if(sign<0) sign = (n%2)?(-1):(1);
           return VpSetZero((VP_HANDLE)y,sign);
        }
    }
    if(VpIsNaN((VP_HANDLE)x)) return VpException(VpSetNaN((VP_HANDLE)y),"Result NaN in VpPower()!");
    if(VpIsInf((VP_HANDLE)x)) {
        if(n==0) return VpSetOne((VP_HANDLE)y);
        if(n>0) return  VpException(VpSetInf((VP_HANDLE)y, (n%2==0 || VpIsPosInf((VP_HANDLE)x)) ? 1 : -1),"Result Infinity in VpPower()!");
        return VpSetZero((VP_HANDLE)y, (n%2==0 || VpIsPosInf((VP_HANDLE)x)) ? 1 : -1);
    }

    if((x->exponent == 1) &&(x->Prec == 1) &&(x->frac[0] == 1)) {
        /* abs(x) = 1 */
        VpSetOne((VP_HANDLE)y);
        if(VP_SIGN(x) > 0) goto Exit;
        if((n % 2) == 0) goto Exit;
        VP_FINITE_SIGN(y, -1);
        goto Exit;
    }

    if(n > 0) sign = 1;
    else if(n < 0) {
        sign = -1;
        n = -n;
    } else {
        return VpSetOne((VP_HANDLE)y);
    }

    /* Allocate working variables  */
    hw1 = VpAlloc("0",(y->MaxPrec + 2) * BASE_FIG+1);
    if(VpIsInvalid(hw1)) return VpException(VpSetNaN(hy),"Memory allocation error in VpPower()");
    w1 = (Real*)hw1;
    hw2 = VpAlloc("0",(w1->MaxPrec * 2 + 1) * BASE_FIG+1);
    if(VpIsInvalid(hw2)) {
        VpFree(&hw1);
        return VpException(VpSetNaN(hy),"Memory allocation error in VpPower()");
    }
    w2 = (Real*)hw2;

    /* calculation start */
    VpAsgn((VP_HANDLE)y, (VP_HANDLE)x, 1);
    --n;
    while(n > 0) {
        VpAsgn((VP_HANDLE)w1, (VP_HANDLE)x, 1);
        s = 1;
        while (ss = s, (s += s) <= (VP_UINT)n) {
            VpMul((VP_HANDLE)w2, (VP_HANDLE)w1, (VP_HANDLE)w1);
            VpAsgn((VP_HANDLE)w1, (VP_HANDLE)w2, 1);
        }
        n -= (int  )ss;
        VpMul((VP_HANDLE)w2, (VP_HANDLE)y, (VP_HANDLE)w1);
        VpAsgn((VP_HANDLE)y, (VP_HANDLE)w2, 1);
    }
    if(sign < 0) {
        VpDiv((VP_HANDLE)w1, (VP_HANDLE)w2, (VP_HANDLE)VpConstOne, (VP_HANDLE)y);
        VpAsgn((VP_HANDLE)y, (VP_HANDLE)w1, 1);
    }

Exit:
    VpFree(&hw2);
    VpFree(&hw1);
    return hy;
}

static char *VpAddString(char *t,const char *s)
{
    while(*s) *t++ = *s++;
    *t = 0;
    return t;
}

/*
 *  [Output]
 *   a[]  ... variable to be assigned the value.
 *  [Input]
 *   sign       ... sign of the value
 *   int_chr[]  ... integer part(not include '+/-').
 *   ni         ... number of characters in int_chr[],not including '+/-'.
 *   frac[]     ... fraction part.
 *   nf         ... number of characters in frac[].
 *   signe      ... sign of the exponent part.
 *   exp_chr[]  ... exponent part(including '+/-').
 *   ne         ... number of characters in exp_chr[],not including '+/-'.
 *  [returns]
 *   VP_HANDLE  ... success
 *   ERROR code ... Real *p is freed before returnning to the caller.
 */
static VP_HANDLE 
    VpCtoV(Real *a, int sign, const char *int_chr, VP_UINT ni, const char *frac, VP_UINT nf, int signe, const char *exp_chr, VP_UINT ne)
{
    VP_UINT   i, j, ind_a, ma;
    int e, es, eb, ef;

    /* get exponent part */
    e  = 0;
    ma = a->MaxPrec;

    for (i=0;i<ne;++i) {
        es = e;
        e  = e * 10 + (int)(exp_chr[i] - '0');
        if (es > (int)(e*BASE_FIG)) {
            /* Exponent overflow */
            int zero = 1;
            for (     ; i < ni && zero; i++) {
                if(int_chr[i]==gDigitSeparator) {++ni;continue;}
                zero = (int_chr[i] == '0');
            }
            for (i = 0; i < nf && zero; i++) {
                if(frac[i]==gDigitSeparator) {++nf;continue;}
                zero = frac[i]    == '0';
            }
            if (!zero && signe > 0) {
                VpSetInf((VP_HANDLE)a, sign);
            }
            else VpSetZero((VP_HANDLE)a, sign);
            return (VP_HANDLE)a;
        }
    }

    e = signe * e;          /* e: The value of exponent part. */
    e = e + (int)ni;  /* set actual exponent size. */

    /* Adjust the exponent so that it is the multiple of BASE_FIG. */
    j = 0;
    ef = 1;
    while (ef) {
        if (e >= 0) eb =  e;
        else        eb = -e;
        ef = eb / (int)BASE_FIG;
        ef = eb - ef * (int)BASE_FIG;
        if (ef) {
            ++j;        /* Means to add one more preceeding zero */
            ++e;
        }
    }

    eb = e / (int)BASE_FIG;

    ind_a = 0;
    i     = 0;
    a->frac[ind_a] = 0;
    while (i < ni) {
        while ((j < BASE_FIG) && (i < ni)) {
            if (int_chr[i]==gDigitSeparator) {++i;++ni;continue;}
            a->frac[ind_a] = a->frac[ind_a] * 10 + (VP_DIGIT)(int_chr[i] - '0');
            ++j;
            ++i;
        }
        if (i < ni) {
            ++ind_a;
            ASSERT(ind_a <= ma); /* Because a->frac has (ma+1) elements */
            j = 0;
        }
   }

    /* get fraction part */
    i = 0;
    while (i < nf) {
        while ((j < BASE_FIG) && (i < nf)) {
            if (frac[i]==gDigitSeparator) {++i;++nf;continue;}
            a->frac[ind_a] = a->frac[ind_a] * 10 + (VP_DIGIT)(frac[i] - '0');
            ++j;
            ++i;
        }
        if (i < nf) {
            ++ind_a;
            ASSERT(ind_a <= ma);  /* Because a->frac has (ma+1) elements */
            j = 0;
        }
    }
    ASSERT(ind_a <= ma);  /* Because a->frac has (ma+1) elements */
    while (j < BASE_FIG) {
        a->frac[ind_a] = a->frac[ind_a] * 10;
        ++j;
    }
    a->Prec = ind_a + 1;
    a->exponent = eb;
    VP_FINITE_SIGN(a,sign);
    VpNmlz(a);
    ASSERT(a->Prec<=a->MaxPrec);
    return (VP_HANDLE)a;
}

/*
   +++++++++++++++++++++++++++++++ Exported functions. ++++++++++++++++++++++++++++++++++
*/

 VP_EXPORT(VP_UINT) VpGetMaxIterationCount()          {return gMaxIter;}
 VP_EXPORT(VP_UINT) VpSetMaxIterationCount(VP_UINT c) {return gMaxIter=c;}
 VP_EXPORT(VP_UINT) VpGetIterationCount()             {return gIterCount;}


VP_EXPORT(int) VpGetRoundMode() {return gRoundMode;}
VP_EXPORT(int) VpSetRoundMode(int m) {
    switch(m) {
    case VP_ROUND_UP:
    case VP_ROUND_DOWN:
    case VP_ROUND_HALF_UP:
    case VP_ROUND_HALF_DOWN:
    case VP_ROUND_CEIL:
    case VP_ROUND_FLOOR:
    case VP_ROUND_HALF_EVEN:
        gRoundMode = m;
        break;
    default: ASSERT(FALSE);
        break;
    }
    return gRoundMode;
}
VP_EXPORT(VP_UINT) VpGetDigitSeparationCount() {return gDigitSeparationCount;}
VP_EXPORT(VP_UINT) VpSetDigitSeparationCount(VP_UINT m) {if(m<=10000) gDigitSeparationCount = m;return gDigitSeparationCount;}
VP_EXPORT(char)    VpGetDigitSeparator() {return gDigitSeparator;}
VP_EXPORT(char)    VpSetDigitSeparator(char c) {
    switch(c) {
    case '\0':
    case '+':
    case '-':
        return gDigitSeparator;
    }
    if(!isdigit(c)) gDigitSeparator = c;
    return gDigitSeparator;
}
VP_EXPORT(char) VpGetDigitLeader() {return gDigitLeader;}
VP_EXPORT(char) VpSetDigitLeader(char c) {
    switch(c) {
    case '\0':
    case ' ':
    case '+':
        gDigitLeader = c;
        break;
    default:
        ASSERT(FALSE);
    }
    return gDigitLeader;
}

/* Sign */
/* Change sign of a to a>0,a<0 if s = 1,-1 respectively */
VP_EXPORT(VP_HANDLE) VpSetSign(VP_HANDLE a,int s)    {if((s)>0) ((Real*)a)->sign=(int)Abs((int)((Real*)a)->sign);else ((Real*)a)->sign=-(int)Abs((int)((Real*)a)->sign);return a;}
VP_EXPORT(int)       VpGetSign(VP_HANDLE a)          {return (int)((Real*)a)->sign;}
VP_EXPORT(VP_HANDLE) VpRevertSign(VP_HANDLE a)       {((Real*)a)->sign=-(((Real*)a)->sign);return a;}
VP_EXPORT(VP_HANDLE) VpAbs(VP_HANDLE c)       {((Real*)c)->sign=(((Real*)c)->sign)>0?((Real*)c)->sign:-((Real*)c)->sign;return c;}

/* 1 */
VP_EXPORT(VP_HANDLE) VpSetOne(VP_HANDLE a)       {
    ((Real*)a)->Prec=1;
    ((Real*)a)->exponent=1;
    ((Real*)a)->frac[0]=1;
    ((Real*)a)->sign=VP_SIGN_POSITIVE_FINITE;
    return a;
}

/* ZEROs */
VP_EXPORT(int)       VpIsPosZero(VP_HANDLE a)     { return (((Real*)a)->sign==VP_SIGN_POSITIVE_ZERO);}
VP_EXPORT(int)       VpIsNegZero(VP_HANDLE a)     { return (((Real*)a)->sign==VP_SIGN_NEGATIVE_ZERO);}
VP_EXPORT(int)       VpIsZero(VP_HANDLE a)        { return (VpIsPosZero(a) || VpIsNegZero(a));}
VP_EXPORT(VP_HANDLE) VpSetPosZero(VP_HANDLE a)    {((Real*)a)->frac[0]=0;((Real*)a)->Prec=1;((Real*)a)->sign=VP_SIGN_POSITIVE_ZERO;return a;}
VP_EXPORT(VP_HANDLE) VpSetNegZero(VP_HANDLE a)    {((Real*)a)->frac[0]=0;((Real*)a)->Prec=1;((Real*)a)->sign=VP_SIGN_NEGATIVE_ZERO;return a;}
VP_EXPORT(VP_HANDLE) VpSetZero(VP_HANDLE a,int s) {(((s)>0)?VpSetPosZero(a):VpSetNegZero(a));return a;}

/* NaN */
VP_EXPORT(int)       VpIsNaN(VP_HANDLE a)     {return (((Real*)a)->sign==VP_SIGN_NaN);}
VP_EXPORT(VP_HANDLE) VpSetNaN(VP_HANDLE a)    {((Real*)a)->frac[0]=0;((Real*)a)->Prec=1;((Real*)a)->sign=VP_SIGN_NaN;return a;}

/* Infinity */
VP_EXPORT(int)       VpIsPosInf(VP_HANDLE a)  { return (((Real*)a)->sign==VP_SIGN_POSITIVE_INFINITE);}
VP_EXPORT(int)       VpIsNegInf(VP_HANDLE a)  { return (((Real*)a)->sign==VP_SIGN_NEGATIVE_INFINITE);}
VP_EXPORT(int)       VpIsInf(VP_HANDLE a)     { return (VpIsPosInf(a) || VpIsNegInf(a));}
VP_EXPORT(VP_HANDLE) VpSetPosInf(VP_HANDLE a) { (((Real*)a)->frac[0]=0,((Real*)a)->Prec=1,((Real*)a)->sign=VP_SIGN_POSITIVE_INFINITE);return a;}
VP_EXPORT(VP_HANDLE) VpSetNegInf(VP_HANDLE a) { (((Real*)a)->frac[0]=0,((Real*)a)->Prec=1,((Real*)a)->sign=VP_SIGN_NEGATIVE_INFINITE);return a;}
VP_EXPORT(VP_HANDLE) VpSetInf(VP_HANDLE a,int s)  { ( ((s)>0)?VpSetPosInf(a):VpSetNegInf(a) );return a;}
VP_EXPORT(int)       VpIsOne(VP_HANDLE a)     { return (((Real*)a)->Prec==1 &&((Real*)a)->frac[0]==1 && ((Real*)a)->exponent==1);}
VP_EXPORT(VP_UINT)   VpMaxLength(VP_HANDLE h)  { return ((Real*)h)->MaxPrec*BASE_FIG;}
VP_EXPORT(VP_UINT)   VpCurLength(VP_HANDLE h)    { return ((Real*)h)->Prec*BASE_FIG;}

/*
 return number of effective digits.
*/
VP_EXPORT(VP_UINT)
    VpEffectiveDigits(VP_HANDLE h)
{
    Real *a = (Real*)h;

    if (!VpHasVal((VP_HANDLE)a)) return 0;

    VP_UINT  ex = a->Prec * (int)BASE_FIG;
    VP_DIGIT n = BASE1;
    while ((a->frac[0] / n) == 0) {
         --ex;
         n /= 10;
    }
    n = a->frac[a->Prec-1];
    while(n%10==0)
    {
        ex--;
        n /= 10;
    }
    return ex;
}

static VP_HANDLE
    VpLoadAlloc(const char *szVal,Real *vp,VP_UINT mx)
{
    VP_UINT i, ni, ipf, nf, ipe, ne;
    char v;
    int  sign  = 1; /* sign of the value */
    int  signe = 1; /* sign of exponent part */

    /* skip leading spaces */
    while(isspace(*szVal)) szVal++;

    /* Check on Inf & NaN */
    if (StrCmp(szVal, SZ_PINF) == 0 ||
        StrCmp(szVal, SZ_INF)  == 0 ) {
        if(!vp) vp = (Real*)VpMemAlloc(1);
        if(VpIsInvalid(vp)) return (VP_HANDLE)vp;
        VpSetPosInf((VP_HANDLE)vp);
        return (VP_HANDLE)vp;
    }
    if (StrCmp(szVal, SZ_NINF) == 0) {
        if(!vp) vp = (Real*)VpMemAlloc(1);
        if(VpIsInvalid(vp)) return (VP_HANDLE)vp;
        VpSetNegInf((VP_HANDLE)vp);
        return (VP_HANDLE)vp;
    }
    if (StrCmp(szVal, SZ_NaN) == 0) {
        if(!vp) vp = (Real*)VpMemAlloc(1);
        if(VpIsInvalid(vp)) return (VP_HANDLE)vp;
        VpSetNaN((VP_HANDLE)vp);
        return (VP_HANDLE)vp;
    }

    i = 0;
    if      (szVal[i] == '-') { sign=-1; ++szVal;}
    else if (szVal[i] == '+') {          ++szVal;}

    /* determine integer part */
    ni = 0;        /* digits in mantissa */
    while ((v = szVal[i]) != 0) {
        if (v==gDigitSeparator) {++i;continue;}
        if (!isdigit(v)) break;
        ++i;
        ++ni; /* Num of digits before '.' */
    }

    /* determin fraction part */
    ipf = 0;
    nf  = 0;
    ipe = 0;
    ne  = 0;
    if (v) {
        /* other than digit nor \0 */
        if (szVal[i] == '.') {    /* xxx. */
            ++i;
            ipf = i;
            while ((v = szVal[i]) != 0) {    /* get fraction part. */
                if (v==gDigitSeparator) {++i;continue;}
                if (!isdigit(v)) break;
                ++i;
                ++nf; /* Num of digits after '.' */
            }
        }

        switch (szVal[i]) {
        case '\0':  break;
        case 'e': case 'E':
        case 'd': case 'D':
            ++i;
            v = szVal[i];
            if      (v == '-') {signe = -1; ++i;}
            else if (v == '+')  ++i;
            ipe = i;
            while ((v=szVal[i]) != 0) {
                if (!isdigit(v)) return VpException((VP_HANDLE)VP_ERROR_BAD_STRING,"Bad numeric string.");
                ++i;
                ++ne;
            }
            break;
        default:
            return VpException((VP_HANDLE)VP_ERROR_BAD_STRING,"Bad numeric string.");
            break;
        }
    }
	if(vp) {
		mx = VpMaxLength((VP_HANDLE)vp);
		if(mx<ni + nf + 1) {
            return VpException((VP_HANDLE)VP_ERROR_BAD_HANDLE,"Maximum length too short to load value.");
		}
		memset(vp->frac,0,sizeof(vp->frac[0])*vp->Prec);
	} else {
		mx = Max(ni + nf + 1, mx);
		vp = (Real*)VpMemAlloc(mx);
	}
    if(VpIsInvalid(vp)) return (VP_HANDLE)vp;
    return VpCtoV(vp, sign, szVal, ni, &szVal[ipf], nf, signe, &szVal[ipe], ne);
}


/*
 * Allocates variable.
 * [Input]
 *   szVal ... value(string) assigned. 
 *             If szVal==NULL(or "\0"),then zero is assumed.
 *   mx    ... allocation unit, if it is not enough to hold szVal,then mx is
 *             determined by szVal.
 *             The mx is the number of effective digits can be stored.
 *
 * [Returns]
 *   Pointer to the newly allocated variable, or
 *   ERROR code is returned if memory allocation is failed,or any error. 
 */
VP_EXPORT(VP_HANDLE)
    VpAlloc(const char *szVal,VP_UINT mx)
{
    if((!szVal) || !(*szVal)) {
       /* necessary to be able to store */
       /* at least mx digits. */
       /* szVal==NULL ==> allocate zero value. */
       Real *vp = (Real*)VpMemAlloc(mx);
       if(VpIsInvalid(vp)) return (VP_HANDLE)vp;
       VpSetZero((VP_HANDLE)vp,1);    /* initialize vp to zero. */
       return (VP_HANDLE)vp;
    }
    return VpLoadAlloc(szVal,NULL,mx);
}

/*
 * Load the value represented by szVal to existing vp-variable p.
 * return p,if normally processed,otherwise ERROR code.
 * p must have enough length to keep value given by szVal without any rounding operation,
 * otherwise this function fails.
*/
VP_EXPORT(VP_HANDLE)
	VpLoad(VP_HANDLE p,const char *szVal)
{
    Real *vp = (Real*)p;
    if(VpIsInvalid(vp)|| !szVal) return VpException((VP_HANDLE)VP_ERROR_BAD_HANDLE,"Illegal argument for VpLoad()");
    return VpLoadAlloc(szVal,vp,0);
}


VP_EXPORT(VP_HANDLE)
    VpClone(VP_HANDLE p)
{
    Real *r = (Real*)p;
    ASSERT(p != 0);
    if(r==NULL) return (VP_HANDLE)VP_ERROR_BAD_HANDLE;
    Real *t = (Real*)malloc(r->Size);
    if(t==NULL) return (VP_HANDLE)VP_ERROR_MEMORY_ALLOCATION;
    memcpy(t,r,r->Size);
    gAllocCount++; /* Count up */
    return (VP_HANDLE)t;
}

/*
  Free allocated VP_HANDLE area,and set the handle be NULL.
*/
VP_EXPORT(void)
VpFree(VP_HANDLE *p)
{
    if( p==NULL) return;
    if(*p==0   ) return;
    if(VpIsInvalid(*p)) return;
    free((void *)(*p));
    *p = 0;
    gAllocCount--; /* Count down */
    ASSERT(gAllocCount>=0);
}

/*
 returns allocation count,total number of VP variables not yet freed.
*/
VP_EXPORT(int)
VpAllocCount()
{
    return gAllocCount;
}

/*
  print the value of h to fp in E format.
*/
VP_EXPORT(int)
    VpPrintE(FILE *fp, VP_HANDLE h)
{
    Real *a = (Real *)h;
    VP_UINT i, nc, nd, ZeroSup;
    VP_DIGIT m, e, nn;
    if(VpIsInvalid(h))           return fprintf(fp,"VpPrintF():Invalid handle(%llu)",(unsigned long long)h);
    /* Check if NaN & Inf. */
    if(VpIsNaN((VP_HANDLE)a))    return fprintf(fp,SZ_NaN);
    if(VpIsPosInf((VP_HANDLE)a)) return fprintf(fp,SZ_INF);
    if(VpIsNegInf((VP_HANDLE)a)) return fprintf(fp,SZ_NINF);

    nd = nc = 0;        /*  nd : number of digits in fraction part(every 10 digits, */
    /*    nd<=10). */
    /*  nc : number of caracters printed  */
    ZeroSup = 1;        /* Flag not to print the leading zeros as 0.00xxxxEnn */
    nc = 0;
    if(VP_SIGN(a) < 0) {
        fprintf(fp, "-");
        ++nc;
    } else {
        if(gDigitLeader!=0) {
            nc += fprintf(fp, "%c",gDigitLeader);
        }
    }
    if(VpIsZero((VP_HANDLE)a))   return fprintf(fp,"0.0");

	
	nc += fprintf(fp, "0.");
    for(i=0; i < a->Prec; ++i) {
        m = BASE1;
        e = a->frac[i];
        if(i==(a->Prec-1)) {
            /* Final digits */
            do {
                nn = e/10;
                if(e==nn*10) {
                    e = nn;
                    m /= 10;
                }
            } while(e==nn);
        }
        while(m) {
            nn = e / m;
            if((!ZeroSup) || nn) {
                nc += fprintf(fp, "%lu", (unsigned long)nn);    /* The leading zero(s) */
                ++nd;
                ZeroSup = 0;    /* Set to print succeeding zeros */
            }
            if(gDigitSeparationCount > 1 && nd >= gDigitSeparationCount) {    /* print ' ' after every gDigitSeparationCount digits */
                nd = 0;
                nc += fprintf(fp, "%c",gDigitSeparator);
            }
            e = e - nn * m;
            m /= 10;
        }
    }
    nc += fprintf(fp, "E%d", VpExponent((VP_HANDLE)a));
    return (int)nc;
}

/*
  prints  VP h in F format(xxxxx.yyyyy..).
 */
VP_EXPORT(int)
    VpPrintF(FILE *fp, VP_HANDLE h)
{
    Real *a  = (Real*)h;
    VP_UINT   i, n, nb, nc;
    VP_DIGIT  m, e, nn;
    int       ex;
    int       fdot = 0;

    if(VpIsInvalid(h))  {
        return fprintf(fp,"?? Not VP??");
    }

    /* Check if NaN or Inf. */
    if(VpIsNaN((VP_HANDLE)a))    {return fprintf(fp,SZ_NaN);}
    if(VpIsPosInf((VP_HANDLE)a)) {return fprintf(fp,SZ_INF);}
    if(VpIsNegInf((VP_HANDLE)a)) {return fprintf(fp,SZ_NINF);}

    nc = 0;
    if(VP_SIGN(a) < 0) {
        nc += fprintf(fp, "-");
    } else {
        if(gDigitLeader!=0) {
            nc += fprintf(fp, "%c",gDigitLeader);
        }
    }
    if(VpIsZero((VP_HANDLE)a))   {return fprintf(fp,"0.0");}

    nb = 0;
    n  = a->Prec;
    ex = a->exponent;
    if(ex<=0) {
       nc += fprintf(fp,"0.");
       fdot = 1;
       while(ex<0) {
          for(i=0;i<BASE_FIG;++i) {
                if(gDigitSeparationCount>1 && nb>=gDigitSeparationCount) {
                    nc += fprintf(fp, "%c", (char)gDigitSeparator);
                    nb = 0;
                }
                nc += fprintf(fp,"0");
                nb++;
          }
          ++ex;
       }
       ex = -1;
    } else {
        nb = ex*BASE_FIG;
        m = BASE1;
        e = a->frac[0];
        while(m && !(e/ m) ) {
           m /= 10;
           nb--;
        }
        if(gDigitSeparationCount>1) {
            nb = gDigitSeparationCount - (nb % gDigitSeparationCount);
            if(nb>=gDigitSeparationCount) nb = 0;
        }
    }

    for(i=0;i < n;++i) {
       --ex;
       m = BASE1;
       e = a->frac[i];
       if(i==0 && ex >= 0) {
           int isw = 0;
            while(m) {
                nn = e / m;
                if(nn || isw) {
                    isw = 1;
                    if(gDigitSeparationCount>1 && nb>=gDigitSeparationCount) {
                        if(ex != 0 || (i+1)>=n) nc += fprintf(fp, "%c", (char)gDigitSeparator);
                        nb = 0;
                    }
                    nc += fprintf(fp,"%c",(char)(nn + '0'));
                    nb++;
                }
                e = e - nn * m;
                m /= 10;
            }
       } else {
           while(m) {
                nn = e / m;
                if(gDigitSeparationCount>1 && nb>=gDigitSeparationCount) {
                    if(ex != 0 || (i+1)>=n) nc += fprintf(fp, "%c", (char)gDigitSeparator);
                    nb = 0;
                }
                nb++;
                if( (i+1)==n && e==0 && fdot ) break;
                nc += fprintf(fp,"%c",(char)(nn + '0'));
                e = e - nn * m;
                m /= 10;
           }
       }
       if(ex == 0 && (i+1)<n) {
           nc += fprintf(fp,".");
           nb = 0;
           fdot = 1;
       }
    }
    while(--ex>=0) {
        m = BASE;
        while(m/=10) {
            if(gDigitSeparationCount>1 && nb>=gDigitSeparationCount) {
                nc += fprintf(fp, "%c", (char)gDigitSeparator);
                nb = 0;
            }
            nb++;
            nc += fprintf(fp,"0");
       }
    }
    return nc;
}


/*
 *  returns number of chars needed to represent h in specified format.
 */
VP_EXPORT(VP_UINT)
VpStringLengthF(VP_HANDLE h)
{
    Real *vp = (Real*)h;
    int ex;
    VP_UINT   nc;

    if(vp == NULL)   return 0;
    if(!VpIsNumeric((VP_HANDLE)vp)) return 32; /* not sure,may be OK */

    nc = BASE_FIG*(vp->Prec + 1)+2;
    ex = vp->exponent;
    if(ex < 0) {
         nc += BASE_FIG*(VP_UINT)(-ex);
    }  else {
         if((VP_UINT)ex > vp->Prec) {
             nc += BASE_FIG*((VP_UINT)ex - vp->Prec);
         }
    }
    if(gDigitSeparationCount>1) nc += (nc+gDigitSeparationCount-1)/gDigitSeparationCount;
    return nc;
}

/*
 *  returns number of chars needed to represent h in specified format.
 */
VP_EXPORT(VP_UINT)
VpStringLengthE(VP_HANDLE h)
{
    Real *vp = (Real*)h;
    VP_UINT   nc;
    if(vp == NULL)   return 0;
    if(!VpIsNumeric((VP_HANDLE)vp)) return 32; /* not sure,may be OK */
    nc = BASE_FIG*(vp->Prec + 2)+6; /* 3: sign + exponent chars */
    if(gDigitSeparationCount>1) nc += (nc+gDigitSeparationCount-1)/gDigitSeparationCount;
    return nc;
}

/*
  returns string representation of VP h in E format(0.xxxxx...Eyyyy).
  sz's size must be equal or greater than VpStringLengthE(h). 
*/
VP_EXPORT(char *)
    VpToStringE(VP_HANDLE h,char *sz)
{
    char *psz = sz;
    Real *a   = (Real *)h;
    VP_UINT i, nd, ZeroSup;
    VP_DIGIT m, e, nn;
    if(VpIsInvalid(h))  {
        VpAddString(sz,"?? Not VP??");
        return sz;
    }

    /* Check if NaN or Inf. */
    if(VpIsNaN((VP_HANDLE)a))    {VpAddString(sz,SZ_NaN);return sz;}
    if(VpIsPosInf((VP_HANDLE)a)) {VpAddString(sz,SZ_INF);return sz;}
    if(VpIsNegInf((VP_HANDLE)a)) {VpAddString(sz,SZ_NINF);return sz;}

    nd = 0;        /*  nd : number of digits in fraction part */
    ZeroSup = 1;        /* Flag not to print the leading zeros as 0.00xxxxEnn */

    if(VP_SIGN(a) < 0) {
        psz = VpAddString(psz, "-");
    } else {
        if(gDigitLeader!=0) {
            *psz++ = gDigitLeader;
        }
    }
    if(VpIsZero((VP_HANDLE)a))   {VpAddString(sz,"0.0");return sz;}

	psz = VpAddString(psz,"0.");
    for(i=0; i < a->Prec; ++i) {
        m = BASE1;
        e = a->frac[i];
        if(i==(a->Prec-1)) {
            /* Final digits */
            do {
                nn = e/10;
                if(e==nn*10) {
                    e = nn;
                    m /= 10;
                }
            } while(e==nn);
        }
        while(m) {
            nn = e / m;
            if((!ZeroSup) || nn) {
               *psz++ = '0'+(char)(nn);    /* The leading zero(s) */
                nd++;
                ZeroSup = 0;    /* Set to print succeeding zeros */
            }
            if(gDigitSeparationCount>1 && nd >= gDigitSeparationCount) {    /* print ' ' after every gDigitSeparationCount digits */
                nd = 0;
               *psz++ = gDigitSeparator;
            }
            e = e - nn * m;
            m /= 10;
        }
    }
    /* drop trailing zeros */
    psz--;
    while(*psz=='0' || *psz==gDigitSeparator) psz--;
    sprintf(++psz, "E%d", VpExponent((VP_HANDLE)a));
    return sz;
}

/*
  returns string representation of VP h in F format(xxxxx.yyyyy..).
  sz's size must be equal or greater than VpStringLengthF(h). 
*/
VP_EXPORT(char*)
    VpToStringF(VP_HANDLE h, char *sz)
{
    Real *a  = (Real*)h;
    VP_UINT   i, n;
    VP_DIGIT  m, e, nn;
    char     *psz = sz;
    int       ex;

    if(VpIsInvalid(h))  {
        VpAddString(sz,"?? Not VP??");
        return sz;
    }

    /* Check if NaN or Inf. */
    if(VpIsNaN((VP_HANDLE)a))    {VpAddString(sz,SZ_NaN);return sz;}
    if(VpIsPosInf((VP_HANDLE)a)) {VpAddString(sz,SZ_INF);return sz;}
    if(VpIsNegInf((VP_HANDLE)a)) {VpAddString(sz,SZ_NINF);return sz;}

    if(VP_SIGN(a) < 0) {
        psz = VpAddString(psz, "-");
    } else {
        if(gDigitLeader!=0) {
            *psz++ = gDigitLeader;
        }
    }
    if(VpIsZero((VP_HANDLE)a))   {VpAddString(sz,"0.0");return sz;}

    n  = a->Prec;
    ex = a->exponent;
    if(ex<=0) {
       *psz++ = '0';*psz++ = '.';
       while(ex<0) {
          for(i=0;i<BASE_FIG;++i) {
              *psz++ = '0';
          }
          ++ex;
       }
       ex = -1;
    }

    for(i=0;i < n;++i) {
       --ex;
       if(i==0 && ex >= 0) {
           sprintf(psz, "%lu", (unsigned long)a->frac[i]);
           psz += strlen(psz);
       } else {
           m = BASE1;
           e = a->frac[i];
           while(m) {
               nn = e / m;
               *psz++ = (char)(nn + '0');
               e = e - nn * m;
               m /= 10;
           }
       }
       if(ex == 0) *psz++ = '.';
    }
    while(--ex>=0) {
       m = BASE;
       while(m/=10) *psz++ = '0';
       if(ex == 0) *psz++ = '.';
    }
    *psz = 0;
    while(psz[-1]=='0') *(--psz) = 0; // truncate trailing 0's.
    if(psz[-1]=='.') psz[-1] = 0;     // truncate final '.' if there.

    if(gDigitSeparationCount<=1) return sz;
    /* Now insert gDigitSeparator at every gDigitSeparationCount digit */
    // Find '.' position
    int ixDot = 0;
    while(sz[ixDot]!='.' && sz[ixDot]) ++ixDot;
    n  = strlen(sz);
    if(sz[ixDot]) {
        // fraction part
        i = ixDot;
        nn = 0;
        while(sz[++i]) {
            if(++nn>gDigitSeparationCount) {
                for(VP_UINT j=n;j>=i;--j) sz[j] = sz[j-1];
                sz[i] = gDigitSeparator;
                n++;
                nn = 0;
            }
        }
    }

    // integer part
    i = ixDot;
    nn = 0;
    while(--i>=1) {
        if(!isdigit(sz[i-1]))  break;
        if(++nn>=gDigitSeparationCount) {
            for(VP_UINT j=n;j>=i;--j) sz[j] = sz[j-1];
            if(isdigit(sz[i-1])) sz[i] = gDigitSeparator;
            sz[++n]=0;
            nn = 0;
        }
    }
    return sz;
}


/*
 * Assignment(c=a).
 * [Input]
 *   A   ... RHSV
 *   isw ... switch for assignment.
 *    C = A  when isw > 0 or isw == 0 (not round)
 *    C = -A when isw < 0
 *    if C->MaxPrec < A->Prec,then round operation
 *    will be performed.
 * [Output]
 *  V  ... LHSV
 */
VP_EXPORT(VP_HANDLE)
    VpAsgn(VP_HANDLE C,VP_HANDLE A, int isw) {return VpAsgn2(C,A,isw,VpGetRoundMode());}

VP_EXPORT(VP_HANDLE)
    VpAsgn2(VP_HANDLE C,VP_HANDLE A, int isw,int round)
{
    VP_UINT n;
    int i;
    Real *a = (Real*)A;
    Real *c = (Real*)C;

    if(VpIsNaN((VP_HANDLE)a)) {
        return VpException(VpSetNaN((VP_HANDLE)c),"NaN assigned in VpAsgn()");
    }
    if(VpIsInf((VP_HANDLE)a)) {
        return VpException(VpSetInf((VP_HANDLE)c,isw*VP_SIGN(a)),"Infinity assigned in VpAsgn()");
    }
    if(VpIsZero((VP_HANDLE)a)) {
        return VpSetZero((VP_HANDLE)c,isw*VP_SIGN(a));
    }
    if(C==A) {
        return VpException(VpSetNaN(C),"Invalid argument for VpAsgn(y==x)!");
    }

    c->exponent = a->exponent;    /* store  exponent */
    i = isw;
    if(i==0) i = 1;
    VP_FINITE_SIGN(c,i*VP_SIGN(a));    /* set sign */
    n =(a->Prec < c->MaxPrec) ?(a->Prec) :(c->MaxPrec);
    c->Prec = n;
    memcpy(c->frac, a->frac, n * sizeof(VP_DIGIT));
    if(isw!=0 && c->Prec < a->Prec) {
        VpInternalRound2(c,n,(n>0)?a->frac[n-1]:0,a->frac[n],round);
    }
    return C;
}


VP_EXPORT(VP_HANDLE)
    VpLengthRound(VP_HANDLE p, int ixRound)
{
    return VpLengthRound2(p,ixRound,VpGetRoundMode());
}

VP_EXPORT(VP_HANDLE)
    VpLengthRound2(VP_HANDLE p, int ixRound,int mode)
/*
 * Round from the left hand side of the digits.
 */
{
    Real    *y = (Real*)p;
    VP_DIGIT v;
    if (!VpHasVal((VP_HANDLE)y)) return p; /* Unable to round */
    v = y->frac[0];
    ixRound -= VP_EXPONENT(y)*(int)BASE_FIG;
    while ((v /= 10) != 0) ixRound--;
    ixRound += (int)BASE_FIG-1;
    return VpScaleRound2(p,ixRound,mode);
}

VP_EXPORT(VP_HANDLE)
    VpScaleRound(VP_HANDLE p, int ixRound)
{
    return VpScaleRound2(p,ixRound,VpGetRoundMode());
}

/*
 *
 * nf: digit position for operation.
 *
 */
VP_EXPORT(VP_HANDLE)
    VpScaleRound2(VP_HANDLE p, int ixRound,int mode)
/*
 * Round reletively from the decimal point.
 *   ixRound: digit location to round from the the decimal point.
 *      mode: rounding mode
 */
{
    Real *y = (Real*)p;
    /* fracf: any positive digit under rounding position? */
    int    n,i,ix,ioffset, exptoadd;
    int    shifter;
    VP_DIGIT div,v;

    if (!VpHasVal((VP_HANDLE)y)) return p; /* Unable to round */

    ixRound += y->exponent * (int)BASE_FIG;
    exptoadd=0;
    if (ixRound < 0) {
        /* rounding position too left(large). */
        if((mode!=VP_ROUND_CEIL) && (mode!=VP_ROUND_FLOOR)) {
            VpSetZero((VP_HANDLE)y,VP_SIGN(y)); /* truncate everything */
            return p;
        }
        exptoadd = -ixRound;
        ixRound = 0;
    }

    ix = ixRound / (int )BASE_FIG;
    if ((VP_UINT)ix >= y->Prec) return p;  /* rounding position too right(small). */
    v = y->frac[ix];

    ioffset = ixRound - ix*(int )BASE_FIG;
    n = (int )BASE_FIG - ioffset - 1;
    for (shifter=1,i=0; i<n; ++i) shifter *= 10;

    /* so the representation used (in y->frac) is an array of int , where
       each int  contains a value between 0 and BASE-1, consisting of BASE_FIG
       decimal places.

       v is the value of this int 
       ioffset is the number of extra decimal places along of this decimal digit
          within v.
       n is the number of decimal digits remaining within v after this decimal digit
       shifter is 10**n,
       v % shifter are the remaining digits within v
       v % (shifter * 10) are the digit together with the remaining digits within v
       v / shifter are the digit's predecessors together with the digit
       div = v / shifter / 10 is just the digit's precessors
       (v / shifter) - div*10 is just the digit, which is what v ends up being reassigned to.
    */

    v /= shifter;
    div = v / 10;
    v = v - div*10;
    /* now v is just the digit required.
       now fracf is whether the digit or any of the remaining digits within v are non-zero
    */

    /* drop digits after pointed digit */
    memset(y->frac+ix+1, 0, (y->Prec - (ix+1)) * sizeof(VP_DIGIT));

    switch(mode) {
    case VP_ROUND_UP:
    case VP_ROUND_DOWN:
    case VP_ROUND_HALF_UP:
    case VP_ROUND_HALF_DOWN:
    case VP_ROUND_CEIL:
    case VP_ROUND_FLOOR:
    case VP_ROUND_HALF_EVEN:
        break;
    default: ASSERT(FALSE);
        mode = VpGetRoundMode();
        break;
    }

    switch(mode)
    {
    case VP_ROUND_DOWN: /* Truncate */
         break;
    case VP_ROUND_UP:   /* Roundup */
        if (v!=0) ++div;
    break;
    case VP_ROUND_HALF_UP:
        if (v>=5) ++div;
        break;
    case VP_ROUND_HALF_DOWN:
        if (v > 5) ++div;
        break;
    case VP_ROUND_CEIL:
        if (v!=0 && (VP_SIGN(y)>0)) ++div;
        break;
    case VP_ROUND_FLOOR:
        if (v!=0 && (VP_SIGN(y)<0)) ++div;
        break;
    case VP_ROUND_HALF_EVEN: /* Banker's rounding */
        if (v > 5) ++div;
        else if (v == 5) {
            if (ioffset == 0) {
                /* v is the first decimal digit of its int ;
                   need to grab the previous int  if present
                   to check for evenness of the previous decimal
                   digit (which is same as that of the int  since
                   base 10 has a factor of 2) */
                if (ix && (y->frac[ix-1] % 2)) ++div;
            } else {
                if (div % 2) ++div;
            }
        }
        break;
    default:
        break;
    }

    for (i=0; i<=n; ++i) div *= 10;
    if (div>=BASE) {
        if(ix) {
            y->frac[ix] = 0;
            VpRdup(y,ix);
        } else {
            short s = VP_SIGN(y);
            int   e = y->exponent;
            VpSetOne((VP_HANDLE)y);
            VP_SIGN(y) = s;
            y->exponent = e+1;
        }
    } else {
        y->frac[ix] = div;
        VpNmlz(y);
    }
    if (exptoadd > 0) {
        y->exponent += (int)(exptoadd/BASE_FIG);
        exptoadd %= (int)BASE_FIG;
        for(i=0;i<exptoadd;i++) {
            y->frac[0] *= 10;
            if (y->frac[0] >= BASE) {
                y->frac[0] /= BASE;
                y->exponent++;
            }
        }
    }
    return p;
}

/*
   c = a+b
*/
VP_EXPORT(VP_HANDLE)
    VpAdd(VP_HANDLE c,VP_HANDLE a,VP_HANDLE b)
{
    return VpAddSub((Real*)c,(Real*)a,(Real*)b,1);
}

/*
   c = a-b
*/
VP_EXPORT(VP_HANDLE)
    VpSub(VP_HANDLE c,VP_HANDLE a,VP_HANDLE b)
{
    return VpAddSub((Real*)c,(Real*)a,(Real*)b,-1);
}


/*
 * must be  MaxPrec(c) > Prec(a)+Prec(b) , or NaN is returned.
 *
 *       c = a * b , Where a = a0a1a2 ... an
 *             b = b0b1b2 ... bm
 *             c = c0c1c2 ... cl
 *          a0 a1 ... an   * bm
 *       a0 a1 ... an   * bm-1
 *         .   .    .
 *       .   .   .
 *        a0 a1 .... an    * b0
 *      +_____________________________
 *     c0 c1 c2  ......  cl
 *     nc      <---|
 *     MaxAB |--------------------|
 */
VP_EXPORT(VP_HANDLE)
    VpMul(VP_HANDLE hc, VP_HANDLE ha, VP_HANDLE hb)
{
    Real *c = (Real*)hc;
    Real *a = (Real*)ha;
    Real *b = (Real*)hb;

    VP_UINT   MxIndA, MxIndB, MxIndAB, MxIndC;
    VP_UINT   ind_c, i, ii, nc;
    VP_UINT   ind_as, ind_ae, ind_bs;
    VP_DIGIT  carry;
    VP_DIGIT  s;

    if(!VpIsNumericOP(c,a,b,'*')) return hc; /* No significant digit */

    if(VpIsZero((VP_HANDLE)a) || VpIsZero((VP_HANDLE)b)) {
        /* at least a or b is zero */
        return VpSetZero((VP_HANDLE)c,VP_SIGN(a)*VP_SIGN(b));
    }

    if(VpIsOne((VP_HANDLE)a)) return VpAsgn((VP_HANDLE)c, (VP_HANDLE)b, VP_SIGN(a));
    if(VpIsOne((VP_HANDLE)b)) return VpAsgn((VP_HANDLE)c, (VP_HANDLE)a, VP_SIGN(b));

    if((b->Prec) >(a->Prec)) {
        /* Adjust so that digits(a)>digits(b) */
        Real *w = a;
        a = b;
        b = w;
    }
    MxIndA = a->Prec - 1;
    MxIndB = b->Prec - 1;
    MxIndC = c->MaxPrec - 1;
    MxIndAB = a->Prec + b->Prec - 1;

    if(MxIndC < MxIndAB) {    /* The Max. prec. of c < Prec(a)+Prec(b) */
        return VpException(VpSetNaN(hc),"Unable to multiply due to short capacity of LHSV of VpMul()!");
    }

    /* set LHSV c info */

    c->exponent = a->exponent;    /* set exponent */
    if(!AddExponent(c,b->exponent)) goto Exit;

    VP_FINITE_SIGN(c,VP_SIGN(a)*VP_SIGN(b));    /* set sign  */
    carry = 0;
    nc = ind_c = MxIndAB;
    memset(c->frac, 0, (nc + 1) * sizeof(VP_DIGIT));        /* Initialize c  */
    c->Prec = nc + 1;        /* set precision */
    for(nc = 0; nc < MxIndAB; ++nc, --ind_c) {
        if(nc < MxIndB) {    /* The left triangle of the Fig. */
            ind_as = MxIndA - nc;
            ind_ae = MxIndA;
            ind_bs = MxIndB;
        } else if(nc <= MxIndA) {    /* The middle rectangular of the Fig. */
            ind_as = MxIndA - nc;
            ind_ae = MxIndA -(nc - MxIndB);
            ind_bs = MxIndB;
        } else if(nc > MxIndA) {    /*  The right triangle of the Fig. */
            ind_as = 0;
            ind_ae = MxIndAB - nc - 1;
            ind_bs = MxIndB -(nc - MxIndA);
        }

        for(i = ind_as; i <= ind_ae; ++i) {
            s = a->frac[i] * b->frac[ind_bs--];
            carry = (int )(s / BASE);
            s -= carry * BASE;
            c->frac[ind_c] += (int )s;
            if(c->frac[ind_c] >= BASE) {
                s = c->frac[ind_c] / BASE;
                carry += (int )s;
                c->frac[ind_c] -= (int )(s * BASE);
            }
            if(carry) {
                ii = ind_c;
                while(ii-- > 0) {
                    c->frac[ii] += carry;
                    if(c->frac[ii] >= BASE) {
                        carry = c->frac[ii] / BASE;
                        c->frac[ii] -= (carry * BASE);
                    } else {
                        break;
                    }
                }
            }
        }
    }

Exit:
    VpNmlz(c);
    return hc;
}

/*
 * y = SQRT(x),  y*y - x =>0
 */
VP_EXPORT(VP_HANDLE)
    VpSqrt(VP_HANDLE hy, VP_HANDLE hx)
{
    Real *y = (Real*)hy;
    Real *x = (Real*)hx;
    Real *f = NULL;
    Real *r = NULL;
    VP_HANDLE hf;
    VP_HANDLE hr;
    VP_UINT   y_prec;
    VP_UINT   f_prec;

    VP_UINT   n;
 
    int     prec;
    int     fConverged = 1;

    if(hy==hx || !VpIsNumeric(hx)) {
        return VpException(VpSetNaN(hy),"Invalid argument for VpSqrt(input==output)!");
    }

    /* Zero, NaN or Infinity ? */
    if(!VpHasVal((VP_HANDLE)x)) {
        if(VpIsZero((VP_HANDLE)x)||VP_SIGN(x)>0) return VpAsgn((VP_HANDLE)y,(VP_HANDLE)x,1);
        else                                     return VpException(VpSetNaN((VP_HANDLE)y),"Invalid argument for VpSqrt()!");
    }

     /* Negative ? */
    if(VP_SIGN(x) < 0) return VpException(VpSetNaN((VP_HANDLE)y),"Negative number in VpSqrt()!");

    /* One ? */
    if(VpIsOne((VP_HANDLE)x)) return VpSetOne((VP_HANDLE)y);

    //
    // Initial value
    VpAsgn(hy,hx,1);
    y->exponent /= 2;

    //
    // start Newton method
    //
    n = y->MaxPrec+1;
    if (x->MaxPrec > n) n = x->MaxPrec+1;
    hf = VpAlloc("0",2*(n + 1)*BASE_FIG);
    if(VpIsInvalid(hf)) return VpException(VpSetNaN(hy),"Memory allocation error in VpSqrt()!");
    hr = VpAlloc("0",4*(n + 2)*BASE_FIG);
    if(VpIsInvalid(hr)) {
        VpFree(&hr);
        VpException(VpSetNaN(hy),"Memory allocation error in VpSqrt()!");
    }

    f = (Real*)hf;
    r = (Real*)hr;

    y_prec = y->MaxPrec;
    f_prec = f->MaxPrec;
    prec = x->exponent - y_prec + 1;
    if (x->exponent >= 0) ++prec;
    else                  --prec;

    n = ((DBLE_FIG + BASE_FIG - 1) / BASE_FIG);
    y->MaxPrec = Min(n , y_prec);
    if(y->MaxPrec<y->Prec) y->MaxPrec = y->Prec;
    n = (y_prec * (BASE_FIG+1));
    if (n < gMaxIter) n = gMaxIter;

    gIterCount = 0;
    do {
        y->MaxPrec += BASE_FIG;
        f->MaxPrec = (y->MaxPrec+1)*2;
        if (y->MaxPrec > y_prec) y->MaxPrec = y_prec;
        if (f->MaxPrec > f_prec) f->MaxPrec = f_prec;

        VpDiv((VP_HANDLE)f, (VP_HANDLE)r, (VP_HANDLE)x, (VP_HANDLE)y);    /* f = x/y    */
        VpAddSub(r, f, y, -1);      /* r = f - y  */
        f->MaxPrec = r->Prec + 2;
        if(f->MaxPrec>f_prec)f->MaxPrec = f_prec;
        VpMul((VP_HANDLE)f, (VP_HANDLE)VpConstPt5, (VP_HANDLE)r);         /* f = 0.5*r  */
        if(VpIsZero((VP_HANDLE)f))         goto converge;
        VpAddSub(r, f, y, 1);       /* r = y + f  */
        VpAsgn((VP_HANDLE)y, (VP_HANDLE)r, 1);                            /* y = r      */
        if(f->exponent < prec) goto converge;
    } while(++gIterCount < n);

    VpException(hy,"Iteration count over in VpSqrt()!");
    fConverged = 0;

converge:
    y->MaxPrec = y_prec;
    VpSetSign((VP_HANDLE)y, 1);
    VpNmlz(y);

    VpFree(&hf);
    VpFree(&hr);
    ASSERT(fConverged);
    return hy;
}

#define MEM_CHECK(c,m,msg)  \
if(VpMaxLength(c)<=(m)) {\
    VpFree(&c);\
    c=(VP_HANDLE)VpMemAlloc((m+BASE_FIG));\
    if(VpIsInvalid(c)) {VpException(c,msg);goto ErExit;}\
}

/*
   y = PI
   Calculates 3.1415.... (the number of times that a circle's diameter
                          will fit around the circle) using J. Machin's formula.
*/
VP_EXPORT(VP_HANDLE)
    VpPI(VP_HANDLE pi)
{
    int     exp;
    VP_UINT sig;

    sig    = VpMaxLength(pi)*2+1;
//    sig   += BASE_FIG*2+1;
    pi     = VpSetPosZero(pi);

    VP_HANDLE two    = VpAlloc("2",1);
    VP_HANDLE m25    = VpAlloc("-0.04",4);
    VP_HANDLE m57121 = VpAlloc("-57121",6);

    VP_HANDLE u = VpAlloc("1",sig);
    VP_HANDLE k = VpAlloc("1",sig);
    VP_HANDLE w = VpAlloc("1",sig);
    VP_HANDLE t = VpAlloc("-80",sig);
    VP_HANDLE r = VpAlloc("0",sig+sig+1);

    if(VpIsInvalid(two)||VpIsInvalid(m25)||VpIsInvalid(m57121)||
       VpIsInvalid(u)||VpIsInvalid(k)||VpIsInvalid(w)||VpIsInvalid(t)||VpIsInvalid(r)) {
           VpException(VpSetNaN(pi),"Memory allocation error in VpPI()!");
           goto Exit;
    }

    gIterCount = 0;
    exp = (int)((Real*)pi)->MaxPrec;
    while (!VpIsZero(u) && -(((Real*)u)->exponent) < exp) { 
        VpMul(r,t,m25);
        VpAsgn(t,r,0);
        VpDiv(u,r,t,k);
		VpAdd(r,pi,u);
        VpAsgn(pi,r,0);
        VpAdd(r,k,two);
        VpAsgn(k,r,0);

		if(++gIterCount>gMaxIter) {
            VpException(pi,"Maximum iteration count exhausted in VpPI()");
            goto Exit;
        }
    }

    VpFree(&t);
    t = VpAlloc("956",sig);

    VpSetOne(u);
    VpSetOne(k);
    VpSetOne(w);

    gIterCount = 0;
    while (!VpIsZero(u) && -(((Real*)u)->exponent) < exp) { 
        VpDiv(u,r,t,m57121);
		VpAsgn(t,u,0);
        VpDiv(u,r,t,k);
		VpAdd(r,pi,u);
		VpAsgn(pi,r,0);
        VpAdd(r,k,two);
        VpAsgn(k,r,0);
        if(++gIterCount>gMaxIter) {
            VpException(pi,"Maximum iteration count exhausted in VpPI()");
            goto Exit;
        }
    }

 Exit:
    VpFree(&two);
    VpFree(&m25);
    VpFree(&m57121);
    VpFree(&u);
    VpFree(&k);
    VpFree(&w);
    VpFree(&t);
    VpFree(&r);

    return pi;
}

/*
   y = exp(x)
*/
VP_EXPORT(VP_HANDLE)
    VpExp(VP_HANDLE y,VP_HANDLE x)
{
    VP_HANDLE s;
    VP_HANDLE r;
    VP_HANDLE c;
    VP_HANDLE xn;
    int       sig;
    const char *msg = "Memory allocation error in VpExp()!";

    if(y==x || !VpIsNumeric(x)) {
        return VpException(VpSetNaN(y),"Invalid argument for VpExp()!");
    }

    if(VpIsZero(x))              return VpSetOne(y);
    if (!VpHasVal((VP_HANDLE)x)) return VpException(VpSetNaN(y),"Invalid argument for VpExp()");

    sig = (int)VpMaxLength(y)+BASE_FIG+1;
    s  = VpAlloc("1",sig*2);
    r  = VpAlloc("1",sig*2);
    xn = VpAlloc("1",sig*2);
    c  = VpAlloc("2",1);

    if(VpIsInvalid(s)||VpIsInvalid(r)||VpIsInvalid(xn)||VpIsInvalid(c)) {
           VpException(VpSetNaN(y),msg);
           goto Exit;
    }

    sig = ((Real*)y)->MaxPrec+1;
    
    VpAsgn(xn,x,1);
    VpAsgn(y,x,1);
    VpAdd(y,s,x); /* y = 1+x/1 */

    gIterCount = 0;
    while(-(((Real*)xn)->exponent)<sig) {
        MEM_CHECK(r,VpCurLength(xn)+VpCurLength(x),msg)
        VpMul(r,xn,x);     /* r = x**n/n */
        VpAsgn(s,r,0);    
        VpDiv(xn,r,s,c);   /* xn = xn*x/n! */
        VpAdd(s,y,xn);
        VpAsgn(y,s,0);
        VpRdup((Real*)c,0);
        if(++gIterCount>gMaxIter) {
            VpException(y,"Maximum iteration count exhausted in VpExp()");
            goto Exit;
        }
    }
    goto Exit;

 ErExit:
    VpSetNaN(y);

 Exit:
    VpFree(&xn);
    VpFree(&c);
    VpFree(&s);
    VpFree(&r);
    return y;
}

/*
   y = sin(x)
*/
VP_EXPORT(VP_HANDLE)
    VpSin(VP_HANDLE y,VP_HANDLE x)
{
    VP_HANDLE s;
    VP_HANDLE r;
    VP_HANDLE c;
    VP_HANDLE c2;
    VP_HANDLE xn;
    VP_HANDLE x2;
    int       sig;
    int       sign = 1;

    const char *msg = "Memory allocation error in VpSin()!";

    if(y==x || !VpIsNumeric(x)) {
        return VpException(VpSetNaN(y),"Invalid argument for VpSin()!");
    }

    if(VpIsZero(x))              return VpSetZero(y,VP_SIGN(x));
    if (!VpHasVal((VP_HANDLE)x)) return VpException(VpSetNaN(y),"Invalid argument for VpSin()");

    sig = (int)(((VpMaxLength(y)>VpMaxLength(x))?VpMaxLength(y):VpMaxLength(x))+BASE_FIG+1);
    x2 = VpAlloc("1",(VpCurLength(x)+1)*2);
    s  = VpAlloc("1",sig*2);
    r  = VpAlloc("1",sig*2);
    xn = VpAlloc("1",sig*2);
    c  = VpAlloc("1",2);
    c2 = VpAlloc("1",sig);

    if(VpIsInvalid(s)||VpIsInvalid(r)||VpIsInvalid(xn)||VpIsInvalid(c)||VpIsInvalid(c2)||VpIsInvalid(x2)) {
           VpException(VpSetNaN(y),msg);
           goto Exit;
    }

    sig = ((Real*)y)->MaxPrec+1;
    
    VpMul(x2,x,x);
    VpAsgn(xn,x,1);
    VpAsgn(y,x,1); /* y = x */

    gIterCount = 0;
    sign = 1;
    while(-(((Real*)xn)->exponent)<=sig) {
        MEM_CHECK(r,VpCurLength(xn)+VpCurLength(x2),msg)
        VpMul(r,xn,x2);
        VpAsgn(xn,r,0);
        VpRdup((Real*)c,0);
        VpAsgn(r,c,0);
        VpRdup((Real*)c,0);
        MEM_CHECK(c2,VpCurLength(c)+VpCurLength(r),msg)
        VpMul(c2,c,r);
        VpDiv(s,r,xn,c2);
        VpAsgn(xn,s,0);
        VpAddSub((Real*)r,(Real*)y,(Real*)xn,sign *= -1);
        VpAsgn(y,r,0);
        if(++gIterCount>gMaxIter) {
            VpException(y,"Maximum iteration count exhausted in VpSin()");
            goto Exit;
        }
    }
    goto Exit;

 ErExit:
    VpSetNaN(y);

 Exit:
    VpFree(&x2);
    VpFree(&xn);
    VpFree(&c);
    VpFree(&c2);
    VpFree(&s);
    VpFree(&r);
    return y;
}

/*
   y = cos(x)
*/
VP_EXPORT(VP_HANDLE)
    VpCos(VP_HANDLE y,VP_HANDLE x)
{
    VP_HANDLE s;
    VP_HANDLE r;
    VP_HANDLE c;
    VP_HANDLE c2;
    VP_HANDLE xn;
    VP_HANDLE x2;
    int       sig;
    int       sign = 1;

    const char *msg = "Memory allocation error in VpCos()!";

    if(y==x || !VpIsNumeric(x)) {
        return VpException(VpSetNaN(y),"Invalid argument in VpCos()!");
    }

    if(VpIsZero(x))              return VpSetOne(y);
    if (!VpHasVal((VP_HANDLE)x)) return VpException(VpSetNaN(y),"Invalid argument for VpCos()");

    sig = (int)(((VpMaxLength(y)>VpMaxLength(x))?VpMaxLength(y):VpMaxLength(x))+BASE_FIG+1);
    x2 = VpAlloc("1",(VpCurLength(x)+1)*2);
    s  = VpAlloc("1",sig*2);
    r  = VpAlloc("1",sig*2);
    xn = VpAlloc("1",sig*2);
    c  = VpAlloc("0",2);
    c2 = VpAlloc("1",sig);

    if(VpIsInvalid(s)||VpIsInvalid(r)||VpIsInvalid(xn)||VpIsInvalid(c)||VpIsInvalid(c2)||VpIsInvalid(x2)) {
           VpException(VpSetNaN(y),msg);
           goto Exit;
    }

    sig = ((Real*)y)->MaxPrec+1;
    
    VpMul(x2,x,x);
    VpSetOne(xn);
    VpSetOne(y); /* y = 1 */
    
    ((Real*)c)->sign = VP_SIGN_POSITIVE_FINITE;

    gIterCount = 0;
    sign = 1;
    while(-(((Real*)xn)->exponent)<=sig) {
        MEM_CHECK(r,VpCurLength(xn)+VpCurLength(x2),msg)
        VpMul(r,xn,x2);
        VpAsgn(xn,r,0);
        VpRdup((Real*)c,0);
        VpAsgn(r,c,0);
        VpRdup((Real*)c,0);
        MEM_CHECK(c2,VpCurLength(c)+VpCurLength(r),msg)
        VpMul(c2,c,r);
        VpDiv(s,r,xn,c2);
        VpAsgn(xn,s,0);
        VpAddSub((Real*)r,(Real*)y,(Real*)xn,sign *= -1);
        VpAsgn(y,r,0);
        if(++gIterCount>gMaxIter) {
            VpException(y,"Maximum iteration count exhausted in VpCos()");
            goto Exit;
        }
    }
    goto Exit;

 ErExit:
    VpSetNaN(y);

 Exit:
    VpFree(&x2);
    VpFree(&xn);
    VpFree(&c);
    VpFree(&c2);
    VpFree(&s);
    VpFree(&r);
    return y;
}

/*
   y = arctan(x) -1<=x<= 1  (around |1| ==> not converge !)
*/
VP_EXPORT(VP_HANDLE)
    VpAtan(VP_HANDLE y,VP_HANDLE x)
{
    VP_HANDLE s;
    VP_HANDLE r;
    VP_HANDLE c;
    VP_HANDLE xn;
    VP_HANDLE x2;
    int       sig;
    int       sign = 1;

    const char *msg = "Memory allocation error in VpAtan()!";

    if(y==x || !VpIsNumeric(x)) {
        return VpException(VpSetNaN(y),"Invalid argument for VpAtan()!");
    }

    if(VpIsZero(x))              return VpSetZero(y,VP_SIGN(x));
    if (!VpHasVal((VP_HANDLE)x)) return VpException(VpSetNaN(y),"Invalid argument for VpAtan()");

	if(VpCmp(x,(VP_HANDLE)VpConstNOne)<0) return VpException(VpSetNaN(y),"Invalid argument for VpAtan(x<-1)");
	if(VpCmp(x,(VP_HANDLE)VpConstOne) >0) return VpException(VpSetNaN(y),"Invalid argument for VpAtan(x>1)");

    sig = (int)(((VpMaxLength(y)>VpMaxLength(x))?VpMaxLength(y):VpMaxLength(x))+BASE_FIG+1);
    x2 = VpAlloc("1",(VpCurLength(x)+1)*2);
    s  = VpAlloc("1",sig*2);
    r  = VpAlloc("1",sig*2);
    xn = VpAlloc("1",sig*2);
    c  = VpAlloc("1",2);

    if(VpIsInvalid(s)||VpIsInvalid(r)||VpIsInvalid(xn)||VpIsInvalid(c)||VpIsInvalid(x2)) {
           VpException(VpSetNaN(y),msg);
           goto Exit;
    }

    sig = ((Real*)y)->MaxPrec+1;
    
    VpMul(x2,x,x);
    VpAsgn(xn,x,1);
    VpAsgn(y,x,1); /* y = x */
    
    ((Real*)c)->sign = VP_SIGN_POSITIVE_FINITE;

    gIterCount = 0;
    sign = 1;
    while(-(((Real*)s)->exponent)<=sig) {
        MEM_CHECK(r,VpCurLength(xn)+VpCurLength(x2),msg)
        VpMul(r,xn,x2);
        VpAsgn(xn,r,0);
        VpRdup((Real*)c,0);
        VpRdup((Real*)c,0);
        VpDiv(s,r,xn,c);
        VpAddSub((Real*)r,(Real*)y,(Real*)s,sign *= -1);
        VpAsgn(y,r,0);
        if(++gIterCount>gMaxIter) {
            VpException(y,"Maximum iteration count exhausted in VpAtan()");
            goto Exit;
        }
    }
    goto Exit;

 ErExit:
    VpSetNaN(y);

 Exit:
    VpFree(&x2);
    VpFree(&xn);
    VpFree(&c);
    VpFree(&s);
    VpFree(&r);
    return y;
}

/*
   y = log(x)  0<x<= 2  (around 2 ==> may not converge !)
*/
VP_EXPORT(VP_HANDLE)
    VpLog(VP_HANDLE y,VP_HANDLE x)
{
    VP_HANDLE s;
    VP_HANDLE r;
    VP_HANDLE c;
    VP_HANDLE xn;
    VP_HANDLE x1;
    int       sig;
    int       sign = 1;

    const char *msg = "Memory allocation error in VpLog()!";

    if(y==x || !VpIsNumeric(x)) {
        return VpException(VpSetNaN(y),"Invalid argument for VpLog()!");
    }

    if(VpGetSign(x)<0)                   return VpException(VpSetNaN(y),"Invalid argument for VpLog( Negative )");
	if(VpIsZero(x))                      return VpSetNegInf(y);
	if(VpIsOne(x))                       return VpSetZero(y,VP_SIGN(x));
    if (!VpHasVal((VP_HANDLE)x))         return VpException(VpSetNaN(y),"Invalid argument for VpLog()");
	if(VpCmp(x,(VP_HANDLE)VpConstTwo)>0) return VpException(VpSetNaN(y),"Invalid argument for VpLog(x>2)");

    sig = (int)(((VpMaxLength(y)>VpMaxLength(x))?VpMaxLength(y):VpMaxLength(x))+BASE_FIG+1);
    c  = VpAlloc("1",2);
    x1 = VpAlloc("1",(VpCurLength(x)+1)*2);
    s  = VpAlloc("1",sig*2);
    r  = VpAlloc("1",sig*2);
    xn = VpAlloc("1",sig*2);

    if(VpIsInvalid(s)||VpIsInvalid(r)||VpIsInvalid(xn)||VpIsInvalid(c)||VpIsInvalid(x1)) {
           VpException(VpSetNaN(y),msg);
           goto Exit;
    }

    sig = ((Real*)y)->MaxPrec+1;
    VpSub(x1,x,c);  /* x = x - 1 */
    if(VpCmp(x1,c)>0) {
        VpException(VpSetNaN(y),"Invalid argument for VpLog( x>2 )!");
        goto Exit;
    }
    VpAsgn(xn,x1,1);
    VpAsgn(y,x1,1); /* y = x */
    
    gIterCount = 0;
    sign = 1;
    while(-(((Real*)s)->exponent)<=sig) {
        MEM_CHECK(r,VpCurLength(xn)+VpCurLength(x1),msg)
        VpMul(r,xn,x1);
        VpAsgn(xn,r,0);
        VpRdup((Real*)c,0);
        VpDiv(s,r,xn,c);
        VpAddSub((Real*)r,(Real*)y,(Real*)s,sign *= -1);
        VpAsgn(y,r,0);
        if(++gIterCount>gMaxIter) {
            VpRdup((Real*)y,0);
            VpException(y,"Maximum iteration count exhausted in VpAtan()");
            goto Exit;
        }
    }
    VpRdup((Real*)y,0);
    goto Exit;

 ErExit:
    VpSetNaN(y);

 Exit:
    VpFree(&x1);
    VpFree(&xn);
    VpFree(&c);
    VpFree(&s);
    VpFree(&r);
    return y;
}

#ifdef _DEBUG
void DbgAssert(int  f,const char *file,int line)
{
    if(f) return;
    printf("****** Debug assertion failed %d:%s\n",line,file);
    getchar();
}
#endif // _DEBUG
