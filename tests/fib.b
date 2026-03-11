void main () {

  // Allocate the initial array
  int64[] arr
  int64 size
  size <- 64
  arr <- new Array(size)

  // Initialize the array
  int64 i
  i <- 2
  arr[0] <- 1
  arr[1] <- 1
  while (i < size) :AllocWhile_Body :AllocWhile_Exit
  :AllocWhile_Body
  {
    arr[i] <- 0
  }
  i <- i + 1
  continue
  :AllocWhile_Exit

  // Get the number of values to calculate
  int64 inps
  inps <- input()
  print(inps)
  
  // Loop that many times
  int64 x, y, max_found, total
  x <- 0
  y <- 0
  // Start with 1s in the first cells
  max_found <- 2
  total <- 0
  while (x < inps) :OuterWhile_Body :OuterWhile_Exit
  :OuterWhile_Body
  x <- x + 1
  {
    // Get the fib index to calculate
    int64 fib
    fib <- 0
    fib <- input()

    // If we are requesting a value greater than the current size
    // Grow the array to match it
    if (fib > size) :ArrayTooSmall :ArrayNowGood
    :ArrayTooSmall
    int64 i, new_size
    int64[] new_arr
    i <- 0
    new_size <- fib * 2
    new_arr <- new Array(new_size)
    while (i < new_size) :GrowArray_Body :GrowArray_Exit
    :GrowArray_Body
    {
        new_arr[i] <- 0
        if (i < size) :CopyOldArray :SetZero
        :CopyOldArray
        int64 tmp
        tmp <- arr[i]
        new_arr[i] <- tmp
        :SetZero
    }
    i <- i + 1
    continue
    :GrowArray_Exit
    size <- new_size
    arr <- new_arr

    :ArrayNowGood

    // We can simply start looping from our current max value (wow!)
    y <- max_found

    while (y < fib) :FibWhile_Body :FibWhile_Exit
    :FibWhile_Body
    {
        int64 tmp1, tmp2, res, ind
        tmp1 <- 0
        tmp2 <- 0
        res <- 0
        ind <- 0
        ind <- y - 2
        tmp1 <- arr[ind]
        ind <- y - 1
        tmp2 <- arr[ind]
        res <- tmp1 + tmp2

        // Fake fib so we don't get overflow...
        if (res > 100000) :FakeFibSub :FakeFibGood
        :FakeFibSub
        res <- res - 100000
        :FakeFibGood
        arr[y] <- res
    }
    :FibWhileContinue
    y <- y + 1
    continue
    :FibWhile_Exit

    // Update our max_found and print the value
    if (fib > max_found) :UpdateMaxFound :MaxFoundUpdated
    :UpdateMaxFound
    max_found <- fib
    :MaxFoundUpdated
    fib <- fib - 1
    int64 res
    res <- 0
    res <- arr[fib]
    total <- total + res
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