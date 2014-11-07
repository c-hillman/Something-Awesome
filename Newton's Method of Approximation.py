from urllib import request
from xml.etree import ElementTree
from math import sin, asin, cos, acos, tan, atan, sqrt, log, e, pi #could be used in eval()

def NewtonsMethod(approx, fx, deriv, n=1):
  if n>0:
    new = approx - eval(fx.replace("x", str(approx))) / eval(deriv.replace("x", str(approx)))
    return NewtonsMethod(new, fx, deriv, n-1)
  else:
    return approx

fx = input("Derive: ")
approx = float(input("Root approximation: "))
n = int(input("Number of applications: "))
fxtourl = fx.replace(" ", "%20") #URLs don't like spaces

appid = "XXXX"
query = request.urlopen("http://api.wolframalpha.com/v2/query?input=%22derive%20" + fxtourl + "%22&includepodid=Input&appid=" + appid)
query = query.read().decode("utf-8")

query = ElementTree.fromstring(query)
deriv = query.find("pod/subpod/plaintext").text
deriv = deriv.split(" = ")[1] #query syntax: "d/dx(<input>) = <derivative>"
deriv = deriv.replace(" ", "*").replace("^", "**")

try: print(NewtonsMethod(approx, fx, deriv, n))
except: print("An error occurred in the calculation. Please check the README for possible causes.")
