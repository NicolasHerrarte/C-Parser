proc makeArr() -> int[] {
    init int[] arr <- assign int[3];
    arr[0] = 10;
    arr[1] = 20;
    arr[2] = 30;
    return arr;
}
init int[] result <- makeArr();
init int a <- result[0];
init int b <- result[1];
init int c <- result[2];