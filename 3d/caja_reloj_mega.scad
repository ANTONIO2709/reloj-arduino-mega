/*
  CAJA PARA RELOJ ARDUINO MEGA + TFT
  Diseño paramétrico para Impresión 3D
*/

// --- PARAMETROS ---
ancho_mega = 54;
largo_mega = 102;
alto_caja = 40;
pared = 2;
tolerancia = 0.5;

// --- DIBUJO ---
part = "all"; // [all, base, lid]

if (part == "all" || part == "base") {
    translate([0, 0, 0]) caja_base();
}

if (part == "all" || part == "lid") {
    translate([part=="all" ? largo_mega + 10 : 0, 0, 0]) tapa();
}

module caja_base() {
    difference() {
        // Cuerpo exterior
        cube([largo_mega + pared*2 + tolerancia*2, ancho_mega + pared*2 + tolerancia*2, alto_caja]);
        
        // Vaciado interior
        translate([pared, pared, pared])
            cube([largo_mega + tolerancia*2, ancho_mega + tolerancia*2, alto_caja]);
            
        // Hueco USB y Alimentacion (Ajustar segun necesites)
        translate([-1, pared + 5, pared + 5])
            cube([pared + 2, 12, 11]); // USB
        
        translate([-1, pared + 35, pared + 5])
            cube([pared + 2, 10, 10]); // Jack Alimentacion
            
        // Ranuras de ventilacion (para sensores)
        for (i = [0 : 5]) {
            translate([largo_mega + pared, pared + 10 + i*7, 10])
                cube([pared + 2, 4, 15]);
        }
    }
    
    // Soportes Arduino (Esquinas)
    translate([pared + 5, pared + 5, pared]) cylinder(h=3, r=2, $fn=20);
    translate([largo_mega + pared - 5, pared + 5, pared]) cylinder(h=3, r=2, $fn=20);
    translate([pared + 5, ancho_mega + pared - 5, pared]) cylinder(h=3, r=2, $fn=20);
}

module tapa() {
    difference() {
        // Tapa exterior
        cube([largo_mega + pared*2 + tolerancia*3, ancho_mega + pared*2 + tolerancia*3, pared*2]);
        
        // Ventana para Pantalla TFT
        // Ajustar coordenadas segun la posicion de tu pantalla
        translate([pared + 15, pared + 5, -1])
            cube([60, 45, pared*3]); 
    }
    
    // Reborde para encajar
    difference() {
        translate([pared + tolerancia/2, pared + tolerancia/2, -pared])
            cube([largo_mega + tolerancia, ancho_mega + tolerancia, pared]);
        translate([pared*2, pared*2, -pared-1])
            cube([largo_mega - pared*2, ancho_mega - pared*2, pared+2]);
    }
}
