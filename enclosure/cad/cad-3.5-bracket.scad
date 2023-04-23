
translate([0,0,0])
difference(){
    linear_extrude(height = 3,convexity=10)
        import(file = "cad-3.5-bracket-br-fill.svg");
    linear_extrude(center=true,height = 10,convexity=10)
        import(file = "cad-3.5-bracket-br-hole.svg");
};


translate([0,0,0])
difference(){
    linear_extrude(height = 3,convexity=10)
        import(file = "cad-3.5-bracket-tr-fill.svg");
    linear_extrude(center=true,height = 10,convexity=10)
        import(file = "cad-3.5-bracket-tr-hole.svg");
};


translate([0,0,0])
difference(){
    linear_extrude(height = 3,convexity=10)
        import(file = "cad-3.5-bracket-bl-fill.svg");
    linear_extrude(center=true,height = 10,convexity=10)
        import(file = "cad-3.5-bracket-bl-hole.svg");
};


translate([0,0,0])
difference(){
    linear_extrude(height = 3,convexity=10)
        import(file = "cad-3.5-bracket-tl-fill.svg");
    linear_extrude(center=true,height = 10,convexity=10)
        import(file = "cad-3.5-bracket-tl-hole.svg");
};
