using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace TestsPriorityApp
{
    public partial class BlueprintTesterConfig : Form
    {
        public BlueprintTesterConfig()
        {
            InitializeComponent();

            DataGridViewRow row = new DataGridViewRow();
            row.CreateCells(dataGridView1);  // this line was missing
            row.Cells[0].Value = true;
            row.Cells[1].Value = "BP_Pathfinding_1";
            row.Cells[2].Value = 12;
            row.Cells[3].Value = 1;
            dataGridView1.Rows.Add(row);

        }

        private void dataGridView1_CellContentClick(object sender, DataGridViewCellEventArgs e)
        {

        }

        private void button2_Click(object sender, EventArgs e)
        {

        }

        private void label1_Click(object sender, EventArgs e)
        {

        }

        private void textBox1_TextChanged(object sender, EventArgs e)
        {

        }

        private void label2_Click(object sender, EventArgs e)
        {

        }
    }
}
