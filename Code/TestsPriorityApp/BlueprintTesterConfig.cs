using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
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
        }

        void AddDataGridRow(bool selectedForRun, string name, int minutesSinceModified, string category)
        {
            DataGridViewRow row = new DataGridViewRow();
            row.CreateCells(dataGridView1);
            row.Cells[0].Value = selectedForRun;
            row.Cells[1].Value = name;
            row.Cells[2].Value = minutesSinceModified;
            row.Cells[3].Value = category;
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

        private void label5_Click(object sender, EventArgs e)
        {

        }

        private void textBox2_TextChanged(object sender, EventArgs e)
        {
           
        }

        private void button3_Click(object sender, EventArgs e)
        {
            if (textPathToProject.Text.Length  ==  0 || !System.IO.Directory.Exists(textPathToProject.Text))
            {
                string messageBoxText = "The given path is invalid";
                string caption = "Error";
                System.Windows.Forms.MessageBoxButtons buttons = MessageBoxButtons.OK;

                DialogResult result = MessageBox.Show(messageBoxText, caption, buttons, MessageBoxIcon.Error);

                return;
            }

            string[] allFilesPath = Directory.GetFiles(textPathToProject.Text, "BP_*.uasset", SearchOption.AllDirectories);

            foreach (string file in allFilesPath)
            {
                DateTime modified = File.GetLastWriteTime(file);
                int minutesModifiedSince = (DateTime.Now - modified).Minutes;

                
                string fullPath = Path.GetFullPath(file).TrimEnd(Path.DirectorySeparatorChar);
                
                string resPath = file;
                int indexOfContent = fullPath.IndexOf("Content");
                if (indexOfContent != -1)
                {
                    resPath = fullPath.Substring(indexOfContent + "Content".Length + 1);
                }


                string category = "Gameplay";
                if (resPath.Contains("AI"))
                {
                    category = "AI";
                }
                else if (resPath.Contains("Crowd") || resPath.Contains("Characters"))
                {
                    category = "Animation";
                }
                else if (resPath.Contains("UI"))
                {
                    category = "UI";
                }

                AddDataGridRow(true, resPath, minutesModifiedSince, category);
            }
        }

        private void BlueprintTesterConfig_Load(object sender, EventArgs e)
        {

        }
    }
}
