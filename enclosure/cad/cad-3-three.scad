
translate([0,0,0])
difference(){
    linear_extrude(height = 3,convexity=10) 
        import(file = "cad-3-three-fill.svg");
    linear_extrude(center=true,height = 10,convexity=10) 
        import(file = "cad-3-three-hole.svg");
    translate([55,20,-3])
    cube([15,68.4,10]);
};

