module C #(
  parameter layer="F.Cu",
  parameter reference="REF**",
  parameter value="0")
  (inout A, inout B);

  (* blackbox *)
  (* kicad_lib="Capacitor_SMD" *)
  C_0805_2012Metric #(
    .value(value),
    .layer(layer),
    .reference(reference)
  ) _TECHMAP_REPLACE_ (A, B);

endmodule

module R #(
  parameter layer="F.Cu",
  parameter reference="REF**",
  parameter value="0")
  (inout A, inout B);

  (* blackbox *)
  (* kicad_lib="Resitor_SMD" *)
  R_0805_2012Metric #(
    .value(value),
    .layer(layer),
    .reference(reference)
  ) _TECHMAP_REPLACE_ (A, B);

endmodule