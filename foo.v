module foo;

wire Vcc, Vdrive, GND;

R #(.value("100k")) r1(Vcc, Vdrive);
C #(.value("10u")) c1(Vdrive, GND);

endmodule