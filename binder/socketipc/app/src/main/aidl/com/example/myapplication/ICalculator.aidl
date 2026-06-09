package com.example.myapplication;

interface ICalculator {
    int add(in int a, in int b);
    String getProtocol();
    byte[] processCustomPayload(in byte[] payload);
    byte[] processParcelPayload(in byte[] payload);
}
