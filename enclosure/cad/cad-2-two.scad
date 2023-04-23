translate([0,0,0])
difference(){
    linear_extrude(height = 3,convexity=10)
        import(file = "cad-2-two-fill.svg");
    linear_extrude(center=true,height = 10,convexity=10)
        import(file = "cad-2-two-hole.svg");
};