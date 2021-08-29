def handle_wax_request(request, response, waxhub):
  num = request.get('num')
  
  output = []
  err = None
  if num == None:
    err = "num field not set"
  elif type(1) != type(num):
    err = "num field is not an integer"
  elif num < 1:
    err = "Must be a positive integer greater than 2"
  else:
    isEven = False
    while num % 2 == 0:
      isEven = True
      output.append(2)
      num = num // 2
    
    waxhub.printroot("My preliminary finding is that " + str(num) + " is an " + ("EVEN" if isEven else "ODD") + " number.")

    div = 3
    while num > 1:
      while num % div == 0:
        output.append(div)
        num //= div
      div += 2
    
  response.send({
    "err": err,
    "factors": output
  })
