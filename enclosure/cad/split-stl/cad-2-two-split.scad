
//intersection(){
difference(){

translate([0,0,0])
difference(){
    linear_extrude(height = 2,convexity=10)
        import(file = "cad-2-two-fill.svg");
    linear_extrude(center=true,height = 12,convexity=10)
        import(file = "cad-2-two-hole.svg");
};

translate([0,0,-10])union()
{
    cube([55,186,20]);
    cube([180,126,20]);
};
}