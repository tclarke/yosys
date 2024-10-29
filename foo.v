(* board_thickness="1.6" *)
module foo;

wire Vcc, Vdrive, GND;

R #(.value("100k")) r1(Vcc, Vdrive);
C #(.value("10u")) c1(Vdrive, GND);

endmodule