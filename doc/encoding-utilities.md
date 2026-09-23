# Encoding Utilities

It is generally important to make LoRaWAN messages as small as practical. Extra bytes mean extra transmit time, which wastes battery power and interferes with other nodes on the network.

To simplify coding, the Arduino header file <lmic.h> defines some data encoding utility functions to encode floating-point data into `uint16_t` values using `sflt16` or `uflt16` bit layout. For even more efficiency, there are versions that use only the bottom 12 bits of the `uint16_t`, allowing for other bits to be carried in the top 4 bits, or for two values to be crammed into three bytes.

- `uint16_t LMIC_f2sflt16(float)` converts a floating point number to a [`sflt16`](#sflt16)-encoded `uint16_t`.
- `uint16_t LMIC_f2uflt16(float)` converts a floating-point number to a [`uflt16`](#uflt16)-encoded `uint16_t`.
- `uint16_t LMIC_f2sflt12(float)` converts a floating-point number to a [`sflt12`](#sflt12)-encoded `uint16_t`, leaving the top four bits of the result set to zero.
- `uint16_t LMIC_f2uflt12(float)` converts a floating-point number to a [`uflt12`](#sflt12)-encoded `uint16_t`, leaving the top four bits of the result set to zero.

JavaScript code for decoding the data can be found in the following sections.

## sflt16

A `sflt16` datum represents an unsigned floating point number in the range [0, 1.0), transmitted as a 16-bit field. The encoded field is interpreted as follows:

bits | description
:---:|:---
15 | Sign bit
14..11 | binary exponent `b`
10..0 | fraction `f`

The corresponding floating point value is computed by computing `f`/2048 * 2^(`b`-15). Note that this format is deliberately not IEEE-compliant; it's intended to be easy to decode by hand and not overwhelmingly sophisticated. However, it is similar to IEEE format in that it uses sign-magnitude rather than twos-complement for negative values.

For example, if the data value is 0x8D, 0x55, the equivalent floating point number is found as follows.

1. The full 16-bit number is 0x8D55.
2. Bit 15 is 1, so this is a negative value.
3. `b`  is 1, and `b`-15 is -14.  2^-14 is 1/16384
4. `f` is 0x555. 0x555/2048 = 1365/2048 is 0.667
5. `f * 2^(b-15)` is therefore 0.667/16384 or 0.00004068
6. Since the number is negative, the value is -0.00004068

Floating point mavens will immediately recognize:

* This format uses sign/magnitude representation for negative numbers.
* Numbers do not need to be normalized (although in practice they always are).
* The format is somewhat wasteful, because it explicitly transmits the most-significant bit of the fraction. (Most binary floating-point formats assume that `f` is is normalized, which means by definition that the exponent `b` is adjusted and `f` is shifted left until the most-significant bit of `f` is one. Most formats then choose to delete the most-significant bit from the encoding. If we were to do that, we would insist that the actual value of `f` be in the range 2048..4095, and then transmit only `f - 2048`, saving a bit. However, this complicates the handling of gradual underflow; see next point.)
* Gradual underflow at the bottom of the range is automatic and simple with this encoding; the more sophisticated schemes need extra logic (and extra testing) in order to provide the same feature.

### JavaScript decoder

```javascript
function sflt162f(rawSflt16)
    {
    // rawSflt16 is the 2-byte number decoded from wherever;
    // it's in range 0..0xFFFF
    // bit 15 is the sign bit
    // bits 14..11 are the exponent
    // bits 10..0 are the the mantissa. Unlike IEEE format,
    // the msb is explicit; this means that numbers
    // might not be normalized, but makes coding for
    // underflow easier.
    // As with IEEE format, negative zero is possible, so
    // we special-case that in hopes that JavaScript will
    // also cooperate.
    //
    // The result is a number in the open interval (-1.0, 1.0);
    //

    // throw away high bits for repeatability.
    rawSflt16 &= 0xFFFF;

    // special case minus zero:
    if (rawSflt16 == 0x8000)
        return -0.0;

    // extract the sign.
    var sSign = ((rawSflt16 & 0x8000) != 0) ? -1 : 1;

    // extract the exponent
    var exp1 = (rawSflt16 >> 11) & 0xF;

    // extract the "mantissa" (the fractional part)
    var mant1 = (rawSflt16 & 0x7FF) / 2048.0;

    // convert back to a floating point number. We hope
    // that Math.pow(2, k) is handled efficiently by
    // the JS interpreter! If this is time critical code,
    // you can replace by a suitable shift and divide.
    var f_unscaled = sSign * mant1 * Math.pow(2, exp1 - 15);

    return f_unscaled;
    }
```

## uflt16

A `uflt16` datum represents an unsigned floating point number in the range [0, 1.0), transmitted as a 16-bit field. The encoded field is interpreted as follows:

bits | description
:---:|:---
15..12 | binary exponent `b`
11..0 | fraction `f`

The corresponding floating point value is computed by computing `f`/4096 * 2^(`b`-15). Note that this format is deliberately not IEEE-compliant; it's intended to be easy to decode by hand and not overwhelmingly sophisticated.

For example, if the transmitted message contains 0xEB, 0xF7, and the transmitted byte order is big endian, the equivalent floating point number is found as follows.

1. The full 16-bit number is 0xEBF7.
2. `b`  is therefore 0xE, and `b`-15 is -1.  2^-1 is 1/2
3. `f` is 0xBF7. 0xBF7/4096 is 3063/4096 == 0.74780...
4. `f * 2^(b-15)` is therefore 0.74780/2 or 0.37390

Floating point mavens will immediately recognize:

* There is no sign bit; all numbers are positive.
* Numbers do not need to be normalized (although in practice they always are).
* The format is somewhat wasteful, because it explicitly transmits the most-significant bit of the fraction. (Most binary floating-point formats assume that `f` is is normalized, which means by definition that the exponent `b` is adjusted and `f` is shifted left until the most-significant bit of `f` is one. Most formats then choose to delete the most-significant bit from the encoding. If we were to do that, we would insist that the actual value of `f` be in the range 4096..8191, and then transmit only `f - 4096`, saving a bit. However, this complicated the handling of gradual underflow; see next point.)
* Gradual underflow at the bottom of the range is automatic and simple with this encoding; the more sophisticated schemes need extra logic (and extra testing) in order to provide the same feature.

### uflt16 JavaScript decoder

```javascript
function uflt162f(rawUflt16)
    {
    // rawUflt16 is the 2-byte number decoded from wherever;
    // it's in range 0..0xFFFF
    // bits 15..12 are the exponent
    // bits 11..0 are the the mantissa. Unlike IEEE format,
    // the msb is explicit; this means that numbers
    // might not be normalized, but makes coding for
    // underflow easier.
    // As with IEEE format, negative zero is possible, so
    // we special-case that in hopes that JavaScript will
    // also cooperate.
    //
    // The result is a number in the half-open interval [0, 1.0);
    //

    // throw away high bits for repeatability.
    rawUflt16 &= 0xFFFF;

    // extract the exponent
    var exp1 = (rawUflt16 >> 12) & 0xF;

    // extract the "mantissa" (the fractional part)
    var mant1 = (rawUflt16 & 0xFFF) / 4096.0;

    // convert back to a floating point number. We hope
    // that Math.pow(2, k) is handled efficiently by
    // the JS interpreter! If this is time critical code,
    // you can replace by a suitable shift and divide.
    var f_unscaled = mant1 * Math.pow(2, exp1 - 15);

    return f_unscaled;
    }
```

## sflt12

A `sflt12` datum represents an signed floating point number in the range [0, 1.0), transmitted as a 12-bit field. The encoded field is interpreted as follows:

bits | description
:---:|:---
11 | sign bit
11..8 | binary exponent `b`
7..0 | fraction `f`

The corresponding floating point value is computed by computing `f`/128 * 2^(`b`-15). Note that this format is deliberately not IEEE-compliant; it's intended to be easy to decode by hand and not overwhelmingly sophisticated.

For example, if the transmitted message contains 0x8, 0xD5, the equivalent floating point number is found as follows.

1. The full 16-bit number is 0x8D5.
2. The number is negative.
3. `b` is 0x1, and `b`-15 is -14.  2^-14 is 1/16384
4. `f` is 0x55. 0x55/128 is 85/128, or 0.66
5. `f * 2^(b-15)` is therefore 0.66/16384 or 0.000041 (to two significant digits)
6. The decoded number is therefore -0.000041.

Floating point mavens will immediately recognize:

* This format uses sign/magnitude representation for negative numbers.
* Numbers do not need to be normalized (although in practice they always are).
* The format is somewhat wasteful, because it explicitly transmits the most-significant bit of the fraction. (Most binary floating-point formats assume that `f` is is normalized, which means by definition that the exponent `b` is adjusted and `f` is shifted left until the most-significant bit of `f` is one. Most formats then choose to delete the most-significant bit from the encoding. If we were to do that, we would insist that the actual value of `f` be in the range 128 .. 256, and then transmit only `f - 128`, saving a bit. However, this complicates the handling of gradual underflow; see next point.)
* Gradual underflow at the bottom of the range is automatic and simple with this encoding; the more sophisticated schemes need extra logic (and extra testing) in order to provide the same feature.
* It can be strongly argued that dropping the sign bit would be worth the effort, as this would get us 14% more resolution for a minor amount of work.

### sflt12f JavaScript decoder

```javascript
function sflt12f(rawSflt12)
    {
    // rawSflt12 is the 2-byte number decoded from wherever;
    // it's in range 0..0xFFF (12 bits). For safety, we mask
    // on entry and discard the high-order bits.
    // bit 11 is the sign bit
    // bits 10..7 are the exponent
    // bits 6..0 are the the mantissa. Unlike IEEE format,
    // the msb is explicit; this means that numbers
    // might not be normalized, but makes coding for
    // underflow easier.
    // As with IEEE format, negative zero is possible, so
    // we special-case that in hopes that JavaScript will
    // also cooperate.
    //
    // The result is a number in the open interval (-1.0, 1.0);
    //

    // throw away high bits for repeatability.
    rawSflt12 &= 0xFFF;

    // special case minus zero:
    if (rawSflt12 == 0x800)
        return -0.0;

    // extract the sign.
    var sSign = ((rawSflt12 & 0x800) != 0) ? -1 : 1;

    // extract the exponent
    var exp1 = (rawSflt12 >> 7) & 0xF;

    // extract the "mantissa" (the fractional part)
    var mant1 = (rawSflt12 & 0x7F) / 128.0;

    // convert back to a floating point number. We hope
    // that Math.pow(2, k) is handled efficiently by
    // the JS interpreter! If this is time critical code,
    // you can replace by a suitable shift and divide.
    var f_unscaled = sSign * mant1 * Math.pow(2, exp1 - 15);

    return f_unscaled;
    }
```

## uflt12

A `uflt12` datum represents an unsigned floating point number in the range [0, 1.0), transmitted as a 16-bit field. The encoded field is interpreted as follows:

bits | description
:---:|:---
11..8 | binary exponent `b`
7..0 | fraction `f`

The corresponding floating point value is computed by computing `f`/256 * 2^(`b`-15). Note that this format is deliberately not IEEE-compliant; it's intended to be easy to decode by hand and not overwhelmingly sophisticated.

For example, if the transmitted message contains 0x1, 0xAB, the equivalent floating point number is found as follows.

1. The full 16-bit number is 0x1AB.
2. `b`  is therefore 0x1, and `b`-15 is -14.  2^-14 is 1/16384
3. `f` is 0xAB. 0xAB/256 is 0.67
4. `f * 2^(b-15)` is therefore 0.67/16384 or 0.0000408 (to three significant digits)

Floating point mavens will immediately recognize:

* There is no sign bit; all numbers are positive.
* Numbers do not need to be normalized (although in practice they always are).
* The format is somewhat wasteful, because it explicitly transmits the most-significant bit of the fraction. (Most binary floating-point formats assume that `f` is is normalized, which means by definition that the exponent `b` is adjusted and `f` is shifted left until the most-significant bit of `f` is one. Most formats then choose to delete the most-significant bit from the encoding. If we were to do that, we would insist that the actual value of `f` be in the range 256 .. 512, and then transmit only `f - 256`, saving a bit. However, this complicates the handling of gradual underflow; see next point.)
* Gradual underflow at the bottom of the range is automatic and simple with this encoding; the more sophisticated schemes need extra logic (and extra testing) in order to provide the same feature.

### uflt12f JavaScript decoder

```javascript
function uflt12f(rawUflt12)
    {
    // rawUflt12 is the 2-byte number decoded from wherever;
    // it's in range 0..0xFFF (12 bits). For safety, we mask
    // on entry and discard the high-order bits.
    // bits 11..8 are the exponent
    // bits 7..0 are the the mantissa. Unlike IEEE format,
    // the msb is explicit; this means that numbers
    // might not be normalized, but makes coding for
    // underflow easier.
    // As with IEEE format, negative zero is possible, so
    // we special-case that in hopes that JavaScript will
    // also cooperate.
    //
    // The result is a number in the half-open interval [0, 1.0);
    //

    // throw away high bits for repeatability.
    rawUflt12 &= 0xFFF;

    // extract the exponent
    var exp1 = (rawUflt12 >> 8) & 0xF;

    // extract the "mantissa" (the fractional part)
    var mant1 = (rawUflt12 & 0xFF) / 256.0;

    // convert back to a floating point number. We hope
    // that Math.pow(2, k) is handled efficiently by
    // the JS interpreter! If this is time critical code,
    // you can replace by a suitable shift and divide.
    var f_unscaled = mant1 * Math.pow(2, exp1 - 15);

    return f_unscaled;
    }
```
