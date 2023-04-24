
//intersection(){
difference(){
    
difference(){
    linear_extrude(height = 3,convexity=10)
        import(file = "cad-1-top-fill.svg");
    linear_extrude(center=true,height = 10,convexity=10)
        import(file = "cad-1-top-hole.svg");
};

translate([0,0,-10])union()
{
    cube([154,111,20]);
    translate([85,0,0])
    cube([180,201,20]);
};
}