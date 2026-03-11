void main () {
  // Get the number of values to calculate
  int64 inps
  inps <- input()
  print(inps)
  
  // Loop that many times
  int64 x, total
  x <- 0
  total <- 0
  while (x < inps) :OuterWhile_Body :OuterWhile_Exit
  :OuterWhile_Body
  x <- x + 1
  {
    // Get the fib index to calculate
    int64 fib
    fib <- 0
    fib <- input()

    // We can simply start looping from our current max value (wow!)
    
    int64 a, b, y
    a <- 1
    b <- 1
    y <- 0

    int64 limit
    limit <- fib - 2

    while (y < limit) :FibWhile_Body :FibWhile_Exit
    :FibWhile_Body
    {
        int64 tmp
        tmp <- 0
        tmp <- b
        b <- a + b

        // Fake fib so we don't get overflow...
        if (b > 100000) :FakeFibSub :FakeFibGood
        :FakeFibSub
        b <- b - 100000
        :FakeFibGood
        if (tmp > 100000) :FakeFibSub2 :FakeFibGood2
        :FakeFibSub2
        tmp <- tmp - 100000
        :FakeFibGood2

        a <- tmp + 0
    }
    :FibWhileContinue
    y <- y + 1
    continue
    :FibWhile_Exit

    // add the final result to total
    total <- total + b
    // avoid overflow
    if (total > 100000) :FakeTotalSub :FakeTotalGood
    :FakeTotalSub
    total <- total - 100000
    :FakeTotalGood
  }
  continue
  :OuterWhile_Exit

  print(total)

  return
}