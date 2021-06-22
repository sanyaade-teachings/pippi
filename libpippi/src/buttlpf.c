void transfer(z) {
    a = 1.f / (2.f + sqrt(2.f));
    numerator = 1.f + (z^-1);
    numerator *= numerator;
    sub = (2.f - sqrt(2)) / (2.f + sqrt(2));
    denominator = 1.f + (sub * (z^-2));

    result = a * (numerator / denominator);
}
