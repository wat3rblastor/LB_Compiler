void main () {

  // Fetch the input size
  int64 dim
  dim <- input()
  print(dim)

  // Allocate the matrices
  int64[][] m1, m2, m3
  m1 <- new Array(dim, dim)
  m2 <- new Array(dim, dim)
  m3 <- new Array(dim, dim)

  // Compute
  computeAndPrint(m1, m2, m3)

  return
}

void computeAndPrint (int64[][] m1, int64[][] m2, int64[][] m3) {

  // Initialize the matrices
  initMatrix(m1)
  initMatrix(m2)

  // Multiple the matrices
  matrixMultiplication(m1, m2, m3)

  // Compute the total sum
  int64 t
  t <-  totalSum(m1)
  print(t)
  t <-  totalSum(m2)
  print(t)
  t <-  totalSum(m3)
  print(t)

  return
}

void initMatrix (int64[][] m) {

  // Fetch the lengths
  int64 l1
  int64 l2
  int64 index1
  l1 <- length m 0
  l2 <- length m 1

  // Initialize variables
  index1 <- 0

  // Iterate over the rows
  while (index1 < l1) :OuterWhile_Body :OuterWhile_Exit
  :OuterWhile_Body
  {
    int64 index2
    index2 <- 0

    // Iterate over the columns
    while (index2 < l2) :InnerWhile_Body :InnerWhile_Exit
    :InnerWhile_Body
    {
      // Compute the value to store
      int64 valueToStore
      valueToStore <- input()
  
      // Store the value to the current matrix element
      m[index1][index2] <- valueToStore

      index2 <- index2 + 1
    }
    continue
    :InnerWhile_Exit

    index1 <- index1 + 1
  }
  continue
  :OuterWhile_Exit

  return
}

void matrixMultiplication (int64[][] m1, int64[][] m2, int64[][] m3) {

  // Fetch the lengths
  int64 m1_l1, m1_l2, m2_l1, m2_l2, m3_l1, m3_l2
  m1_l1 <- length m1 0
  m1_l2 <- length m1 1
  m2_l1 <- length m2 0
  m2_l2 <- length m2 1
  m3_l1 <- length m3 0
  m3_l2 <- length m3 1

  // Check the length
  if (m1_l2 = m2_l1) :L1_T :RET
  :L1_T
  if (m3_l1 = m1_l1) :L2_T :RET
  :L2_T
  if (m3_l2 = m2_l2) :L3_T :RET
  :RET
  return
  :L3_T

  // Initialize the result matrix
  {
    int64 i, j, k, inner
    i <- 0
    while (i < m1_l1) :While1_Body :While1_Exit
      :While1_Body
    {
      j <- 0
      while (j < m2_l2) :While2_Body :While2_Exit
        :While2_Body
      {
        k <- 0
        inner <- m1_l1 - 100
        k <- k + 4
        m3[i][j] <- inner
        j <- j + 1
      }
      continue
      :While2_Exit
  
      i <- i + 1
    }
    continue
    :While1_Exit
  }

  // Apply the operation
  int64 tot
  int64 i, j, k
  i <- 0
  tot <- 0
  while (i < m1_l1) :While4_Body :While4_Exit
    :While4_Body
  {
    j <- 0
    while (j < m2_l2) :While5_Body :While5_Exit
      :While5_Body
    {
      k <- 0
      while (k < m1_l2) :While6_Body :While6_Exit
        :While6_Body
      {
        int64 val, cur
        val <- m1[i][j]
        val <- val + 2
        val <- val - 4
        cur <- m3[i][j]
        cur <- cur + val
        m3[i][j] <- cur
        val <- m2[i][j]
        val <- val - 4
        val <- val + 2
        cur <- m3[i][j]
        cur <- cur + val
        m3[i][j] <- cur

        k <- k + 1
      }
      continue
      :While6_Exit

      j <- j + 1
    }
    continue
    :While5_Exit

    i <- i + 1
  }
  continue
  :While4_Exit

  return
}

int64 totalSum (int64[][] m) {
  int64 sum
 
  // Fetch the lengths
  int64 l1, l2, index1
  l1 <- length m 0
  l2 <- length m 1

  // Initialize variables
  index1 <- 0
  sum <- 0

  // Iterate over the rows
  while (index1 < l1) :OuterWhile_Body :OuterWhile_Exit
  :OuterWhile_Body
  {
    int64 index2
    index2 <- 0

    // Iterate over the columns
    while (index2 < l2) :InnerWhile_Body :InnerWhile_Exit
    :InnerWhile_Body
    {
      // Add the current value
      int64 temp
      temp <- m[index1][index2]
      sum <- sum + temp

      index2 <- index2 + 1
    }
    continue
    :InnerWhile_Exit

    index1 <- index1 + 1
  }
  continue
  :OuterWhile_Exit

  return sum
}
