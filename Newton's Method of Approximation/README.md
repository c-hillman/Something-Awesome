Possible Error Causes
=====================

There are a few reasons why your input may be causing an error in calculation.

1. Make sure that the hardcoded AppID is a valid Wolfram|Alpha AppID. You can find out more about their API on [their website](http://products.wolframalpha.com/api/).

2. Make sure that the function entered is valid for this program. The function must be able to be derived to a real function that contains only mathematical concepts in the following list:
  * sin/asin
  * cos/acos
  * tan/atan
  * sqrt (ie. the square root function, however fractional indicies are ok)
  * log (natural logarithms, other bases are not valid)
  * e and pi

3. Make sure that appoximation and the number of applications are valid.
  * both must either an integer or a float
  * note that the number of applications is truncated to a integer

4. There are a few limitations to Newton's Method of Approximation that could cause errors:
  * there must actually be a real root
  * there must not be a stationary point between the approximation and the actual root
  * there approximation given cannot be a stationary point
